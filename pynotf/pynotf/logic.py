from abc import ABC, abstractmethod
from collections import deque
from enum import Enum
from logging import error as print_error
from threading import Lock, Condition
from typing import Any, Optional, List, Tuple, Set, Deque, Dict
from weakref import ref as weak_ref

from .value import Value


########################################################################################################################

class CyclicDependencyError(Exception):
    pass


########################################################################################################################

class CompletionSignal:
    """
    Signal emitted by an Emitter that produced the last value in its data stream.
    This is the last Signal to be emitted by that Emitter.
    """

    def __init__(self, emitter_id: int):
        """
        Constructor.
        :param emitter_id:      ID of the Emitter that emitted the Signal.
        """
        self._emitter_id: int = emitter_id  # is constant

    def get_source(self) -> int:
        """
        ID of the Emitter that emitted the Signal.
        """
        return self._emitter_id


########################################################################################################################

class FailureSignal(CompletionSignal):  # protected inheritance in C++
    """
    Signal emitted by an Emitter that failed to produce the next value in its data stream.
    This is the last Signal to be emitted by that Emitter.
    """

    def __init__(self, emitter_id: int, error: Exception):
        """
        Constructor.
        :param emitter_id:      ID of the Emitter that emitted the Signal.
        :param error:           Exception produced by the Emitter.
        """
        CompletionSignal.__init__(self, emitter_id)

        self._error: Exception = error  # is constant

    def get_error(self) -> Exception:
        """
        The exception produced by the Emitter.
        """
        return self._error


########################################################################################################################

class ValueSignal(CompletionSignal):  # protected inheritance in C++
    """
    Signal emitted by an Emitter containing the next value in its data stream.
    """

    class _Status(Enum):
        """
        Status of the Signal with the following state transition diagram:

            --> IGNORED --> ACCEPTED --> BLOCKED
                    |                       ^
                    +-----------------------+
            --> UNBLOCKABLE
        """
        UNBLOCKABLE = 0
        UNHANDLED = 1
        ACCEPTED = 2
        BLOCKED = 3

    def __init__(self, emitter_id: int, value: Value, is_blockable: bool = False):
        """
        Constructor.
        :param emitter_id:      ID of the Emitter that emitted the Signal.
        :param value:           Immutable Value contained in the Signal.
        :param is_blockable:    Whether or not the Signal can be accepted and blocked by Receivers.
        """
        CompletionSignal.__init__(self, emitter_id)

        self._value: Value = value  # is constant
        self._status: ValueSignal._Status = self._Status.UNHANDLED if is_blockable else self._Status.UNBLOCKABLE

    def get_value(self) -> Value:
        """
        Immutable Value contained in the Signal.
        """
        return self._value

    def is_blockable(self) -> bool:
        """
        Whether or not the Signal was at some point able to be accepted and blocked by Receivers.
        """
        return self._status != self._Status.UNBLOCKABLE

    def is_accepted(self) -> bool:
        """
        Returns true iff this Signal is blockable and has been accepted or blocked.
        """
        return self._status in (self._Status.ACCEPTED, self._Status.BLOCKED)

    def is_blocked(self) -> bool:
        """
        Returns true iff this Signal is blockable and has been blocked.
        """
        return self._status == self._Status.BLOCKED

    def accept(self):
        """
        If this Signal is blockable and has not been accepted yet, mark it as accepted.
        """
        if self._status == self._Status.UNHANDLED:
            self._status = self._Status.ACCEPTED

    def block(self):
        """
        If this Signal is blockable and has not been blocked yet, mark it as blocked.
        """
        if self._status in (self._Status.UNHANDLED, self._Status.ACCEPTED):
            self._status = self._Status.BLOCKED


########################################################################################################################

class Emitter:
    """
    Circuit element pushing a Signal downstream.
    Is non-copyable and may only be owned by downstream Receivers and Nodes.
    """

    class Handle:
        """
        An opaque handle to an Emitter that is returned whenever you want to connect to one.
        We cannot just hand over raw references to the actual Emitter object because the user might store them, use
        them to call emit, fail and complete at unexpected times and generally and break a a lot of assumptions that are
        inherent in the design.
        In C++ Handles would be uncopyable.
        """

        def __init__(self, emitter: 'Emitter'):
            """
            Constructor.
            :param emitter: Strong reference to the Emitter represented by this handle.
            """
            self._emitter: weak_ref = weak_ref(emitter)

        def is_valid(self) -> bool:
            """
            Checks if the handle is still valid.
            """
            return self._emitter() is not None

        def get_output_schema(self) -> Optional[Value.Schema]:
            """
            Returns the output schema of the Emitter or None, if the handle is no longer valid.
            """
            emitter: Emitter = self._emitter()
            if emitter is None:
                return None
            else:
                return emitter.get_output_schema()

    class _OrderableReceivers:
        pass

    class _Status(Enum):
        """
        Emitters start out IDLE and change to EMITTING whenever they are emitting a ValueSignal to connected Receivers.
        If an Emitter tries to emit but is already in EMITTING state, we know that we have caught a cyclic dependency.
        To emit a FailureSignal or a CompletionSignal, the Emitter will switch to the respective FAILING or COMPLETING
        state. Once it has finished its `_fail` or `_complete` methods, it will permanently change its state to
        COMPLETED and will not emit anything again.
        We differentiate between EMITTING, FAILING and COMPLETING so that the virtual function `_handle_error` can
        respond differently, depending on the state that the Emitter is currently in.

            --> IDLE <-> EMITTING
                  |
                  +--> FAILING -----+
                  |                 |
                  +--> COMPLETING --+--> COMPLETE
        """
        IDLE = 0
        EMITTING = 1
        FAILING = 2
        COMPLETING = 3
        COMPLETED = 4

    def __init__(self, schema: Value.Schema = Value.Schema(), is_blockable: bool = False):
        """
        Constructor.
        Should only be called by `Circuit.create_emitter` and the Operator constructor.
        :param schema:          Schema of the Value emitted by this Emitter, defaults to empty.
        :param is_blockable:    Whether or not Signals from this Emitter can be blocked, defaults to False.
        """
        self._downstream: List[weak_ref] = []  # subclasses may only change the order
        self._output_schema: Value.Schema = schema  # is constant
        self._is_blockable: bool = is_blockable  # is constant
        self._status: Emitter._Status = self._Status.IDLE

    def get_id(self) -> int:
        """
        Unique identifier of this Circuit element.
        """
        return id(self)

    def get_output_schema(self) -> Value.Schema:
        """
        Schema of the emitted value. Can be the empty Schema if this Emitter does not emit a meaningful value.
        """
        return self._output_schema

    def is_blockable(self) -> bool:
        """
        Whether Signals emitted by this Emitter are blockable or not.
        """
        return self._is_blockable

    def is_completed(self) -> bool:
        """
        Returns true iff the Emitter has been completed, either through an error or normally.
        """
        return self._status == self._Status.COMPLETED

    def has_downstream(self) -> bool:
        """
        Checks if emitting a Value through this Emitter reaches any Receivers downstream.
        This way you can skip expensive calculations of Values if there is nobody to receive them anyway.
        """
        return len(self._downstream) > 0

    # protected
    def _emit(self, value: Any = Value()):
        """
        Push the given value to all active Receivers.
        We offer implicit conversion to a Value here since this code can be called from subclasses by the user.
        :param value:                   Value to emit, can be empty if this Emitter does not emit a meaningful value.
        :raise TypeError:               If the Value's Schema doesn't match.
        :raise CyclicDependencyError:   If the Emitter is already emitting (cyclic dependency detected).
        """
        # Make sure we can never emit once the Emitter has completed.
        if self.is_completed():
            return

        # Check the given value to see if the Emitter is allowed to emit it. If it is not a Value, try to
        # build one out of it. If that doesn't work, or the Schema of the Value does not match that of the
        # Emitter, raise an exception.
        if not isinstance(value, Value):
            try:
                value = Value(value)
            except ValueError:
                raise TypeError("Emitters can only emit Values or things that are implicitly convertible to one")
        if value.schema != self.get_output_schema():
            raise TypeError("Emitter cannot emit a value with the wrong Schema")

        # early out if there wouldn't be a point of emitting anyway
        if not self.has_downstream():
            return

        # Make sure that we are not already emitting.
        if self._status != self._Status.IDLE:  # this would be the job of a RAII guard in C++
            raise CyclicDependencyError(f"Cyclic dependency detected during emission from Emitter {self.get_id()}.")
        self._status = self._Status.EMITTING

        try:
            # sort out invalid receivers and compile a list of strong valid references
            receivers: List[Receiver] = []
            highest_valid_index: int = 0
            for weak_receiver in self._downstream:
                receiver: Optional[Receiver] = weak_receiver()
                if receiver is not None:
                    receivers.append(receiver)
                    self._downstream[highest_valid_index] = weak_receiver  # could be a move or swap in C++
                    highest_valid_index += 1

            # pre-emission allowing user-code to prepare the Emitter (change the order of receivers, for example)
            receiver_order = self._OrderableReceivers()
            self._sort_receivers(receiver_order)

            # if the user changed the order of the strong references, re-apply that order to the member field
            # if receiver_order.has_changed():
            #     self._downstream = [weak_ref(receiver) for receiver in receivers]
            # else:
            # if not, just remove all dead receivers
            self._downstream = self._downstream[:highest_valid_index]

            # create the signal to emit
            signal = ValueSignal(self.get_id(), value, self.is_blockable())  # in C++ w would move the value here

            # emit to all live receivers in order
            for receiver in receivers:
                try:
                    receiver.on_value(signal)
                # pass exceptions to the assigned error handle
                except Exception as exception:
                    self._handle_error(receiver.get_id(), exception)

                # stop the emission if the Signal was blocked
                # C++ will most likely be able to move this condition out of the loop
                if self.is_blockable() and signal.is_blocked():
                    break

        finally:
            # reset the emission flag
            self._status = self._Status.IDLE

    def _fail(self, error: Exception):
        """
        Failure method, completes the Emitter while letting the downstream Receivers inspect the error.
        :param error:           The error that has occurred.
        :raise RuntimeError:    If the Emitter is already emitting (cyclic dependency detected).
        """
        # Make sure we can never emit once the Emitter has completed.
        if self.is_completed():
            # We cannot rule out that a subclass calls this method manually while a "CompletionEvent" is already queued
            # in the Circuit. In that case, the Event might call this function again, in which case we do nothing.
            return

        # make sure that we are not already emitting
        if self._status != self._Status.IDLE:
            raise RuntimeError(f"Cyclic dependency detected during failure from Emitter {self.get_id()}.")
        self._status = self._Status.FAILING

        try:
            # create the signal
            signal: FailureSignal = FailureSignal(self.get_id(), error)

            # emit to all receivers, ignore expired ones but don't remove them
            for weak_receiver in self._downstream:
                receiver: Optional[Receiver] = weak_receiver()
                if receiver is None:
                    continue

                # emit and pass exceptions to the assigned error handle
                try:
                    receiver.on_failure(signal)
                except Exception as exception:
                    self._handle_error(receiver.get_id(), exception)

        finally:
            # complete the emitter
            self._downstream.clear()
            self._status = self._Status.COMPLETED

    def _complete(self):
        """
        Completes the Emitter successfully.
        :raise RuntimeError:    If the Emitter is already emitting (cyclic dependency detected).
        """
        # Make sure we can never emit once the Emitter has completed.
        if self.is_completed():
            # We cannot rule out that a subclass calls this method manually while a "CompletionEvent" is already queued
            # in the Circuit. In that case, the Event might call this function again, in which case we do nothing.
            return

        # make sure that we are not already emitting
        if self._status != self._Status.IDLE:
            raise RuntimeError(f"Cyclic dependency detected during completion from Emitter {self.get_id()}.")
        self._status = self._Status.COMPLETING

        try:
            # create the signal
            signal: CompletionSignal = CompletionSignal(self.get_id())

            # emit to all receivers, ignore expired ones but don't remove them
            for weak_receiver in self._downstream:
                receiver: Optional[Receiver] = weak_receiver()
                if receiver is None:
                    continue

                # emit and pass exceptions to the assigned error handle
                try:
                    receiver.on_completion(signal)
                except Exception as exception:
                    self._handle_error(receiver.get_id(), exception)

        finally:
            # complete the emitter
            self._downstream.clear()
            self._status = self._Status.COMPLETED

    def _get_status(self) -> 'Emitter._Status':
        """
        The current state of the Emitter, can be used by `_handle_error` overloads to determine in what context the
        error occurred.
        Is not part of the public interface since the internal state of the Emitter is of no interest to the user.
        """
        return self._status

    def _handle_error(self, receiver_id: int, error: Exception):  # virtual
        """
        Default error handler.
        Writes a comprehensive error message to std::err.
        Is protected so that overrides of can call the base class implementation.
        :param receiver_id: ID of the Receiver that produced the error.
        :param error:       The exception object.
        """
        assert self._get_status() != self._Status.COMPLETED

        # determine what the Emitter was doing when the error occurred
        if self._get_status() == self._Status.EMITTING:
            state = "emitting a Value"
        elif self._get_status() == self._Status.FAILING:
            state = "failing"
        else:
            assert self._get_status() == self._Status.COMPLETING
            state = "completing"

        # print out the error message
        print_error(f"Receiver {receiver_id} failed during Logic evaluation.\n"
                    f"Exception caught by Emitter {self.get_id()} while {state}:\n"
                    f"{error}")

    # private
    def _sort_receivers(self, receivers: _OrderableReceivers):  # virtual
        """
        Method to allow user-defined code to re-order the Receivers prior to the emission of a value.
        :param receivers: Receivers of this Emitter, can only be re-ordered.
        """
        pass


########################################################################################################################

class Receiver:
    """
    Circuit element receiving a Signal from upstream.
    Is non-copyable and may only be owned by other Receivers downstream and Nodes.
    """

    def __init__(self, circuit: 'Circuit', schema: Value.Schema = Value.Schema()):
        """
        Constructor.
        Should only be called by `Circuit.create_receiver` and the Operator constructor.
        :param circuit: The Circuit that this Receiver is a part of.
        :param schema:  Schema defining the Value expected by this Receiver, defaults to empty.
        """
        self._circuit: Circuit = circuit  # is constant
        self._input_schema: Value.Schema = schema  # is constant
        self._upstream: Set[Emitter] = set()

    def get_id(self) -> int:
        """
        Unique identifier of this Circuit element.
        """
        return id(self)

    def get_input_schema(self) -> Value.Schema:
        """
        Schema of expected values. Can be the empty Schema.
        """
        return self._input_schema

    def has_upstream(self) -> bool:
        """
        Checks if the Receiver has any Emitter connected upstream.
        Useful for overrides of `_on_failure` and `_on_complete` to find out whether the completed Emitter was the last
        connected one.
        """
        return len(self._upstream) != 0

    def on_value(self, signal: ValueSignal):
        """
        Called when an upstream Emitter has produced a new Value.
        :param signal   The Signal associated with this call.
        """
        # assert that the emitter is connected to this receiver
        assert any(emitter.get_id() == signal.get_source() for emitter in self._upstream)

        # it should be impossible to emit a value of the wrong type
        assert signal.get_value().schema == self.get_input_schema()

        # pass the signal along to the user-defined handler
        self._on_value(signal)

    def on_failure(self, signal: FailureSignal):
        """
        Called when an upstream Emitter has failed.
        :param signal       The ErrorSignal associated with this call.
        """
        # assert that the emitter is connected to this receiver
        for candidate in self._upstream:
            if candidate.get_id() == signal.get_source():
                emitter: Emitter = candidate
                break
        else:
            assert False  # received Signal from an Emitter that is not connected

        # since the emitter has already completed, there is no reason to delay disconnecting
        # it will never emit again anyway
        self._upstream.remove(emitter)

        # mark the emitter as ready for deletion
        self._circuit.expire_emitter(emitter)

        # pass the signal along to the user-defined handler
        self._on_failure(signal)

    def on_completion(self, signal: CompletionSignal):
        """
        Called when an upstream Emitter has finished successfully.
        :param signal       The CompletionSignal associated with this call.
        """
        # assert that the emitter is connected to this receiver
        for candidate in self._upstream:
            if candidate.get_id() == signal.get_source():
                emitter: Emitter = candidate
                break
        else:
            assert False  # received Signal from an Emitter that is not connected

        # since the emitter has already completed, there is no reason to delay disconnecting
        # it will never emit again anyway
        self._upstream.remove(emitter)

        # mark the emitter as ready for deletion
        self._circuit.expire_emitter(emitter)

        # pass the signal along to the user-defined handler
        self._on_completion(signal)

    def connect_to(self, emitter: Emitter.Handle):
        """
        Connect to the given Emitter.
        In C++, the emitter argument would need to be an r-value, because handles are non-copyable.
        :param emitter:     Emitter to connect to. If this Receiver is already connected, this does nothing.
        :raise TypeError:   If the Emitters's input Schema does not match.
        """
        # if the Emitter from the Handle is already expired, do nothing
        strong_emitter: Optional[Emitter] = emitter._emitter()
        if strong_emitter is None:
            return

        # fail immediately if the schemas don't match
        if self.get_input_schema() != strong_emitter.get_output_schema():
            raise TypeError(f"Cannot connect a Emitter {strong_emitter.get_id()} to Receiver {self.get_id()} "
                            f"which has a different Value Schema")

        # store the operator right away to keep it alive, this does not count as a connection yet
        self._upstream.add(strong_emitter)

        # schedule the creation of the connection
        self._circuit.create_connection(strong_emitter.get_id(), self)

    def create_operator(self, operation: 'Operator.Operation', name: str = ""):
        """
        Creates and connects to a new Operator upstream.
        :param operation: Operation defining the Operator.
        :param name: Name under which the Operator should be registered.
        :raise TypeError: If the output type of the Operation does not match this Receiver's.
        :raise NameError: If another Operator with the same name is already registered.
        """
        # fail immediately if the schemas don't match
        if self.get_input_schema() != operation.get_output_schema():
            raise TypeError(f"Cannot create an Operation to connect to Receiver {self.get_id()} "
                            f"which has a different Value Schema")

        # create the Operator
        operator: Operator = Operator(self._circuit, operation)

        # make sure the name is not already taken by another living Emitter
        if name:
            with self._circuit._name_mutex:
                existing: Optional[weak_ref] = self._circuit._named_emitters.get(name, None)
                if existing is not None and existing() is not None:
                    raise NameError(f"Failed to create an upstream Operator from Circuit element {self.get_id()}. "
                                    f"Another Emitter with the name {name} already exists.")
                else:
                    # register a named emitter with the circuit
                    self._circuit._named_emitters[name] = weak_ref(operator)

        # store the operator right away to keep it alive, this does not count as a connection yet
        self._upstream.add(operator)

        # the actual connection is delayed until the end of the event
        self._circuit.create_connection(operator.get_id(), self)

    def find_emitter(self, name: str) -> Optional[Emitter.Handle]:
        """
        Finds and returns a named Emitter from the Circuit, if one exists.
        :param name:    Name of the Emitter to find.
        :return:        The requested Emitter or None.
        """
        with self._circuit._name_mutex:
            weak_emitter: Optional[weak_ref] = self._circuit._named_emitters.get(name, None)
            if weak_emitter is None:
                return None  # no Emitter by that name

            emitter: Optional[Emitter] = weak_emitter()
            if emitter is None:
                del self._circuit._named_emitters[name]
                return None  # Emitter has expired

        return Emitter.Handle(emitter)

    def disconnect_from(self, emitter_id: int):
        """
        Disconnects from the given Emitter.
        :param emitter_id: ID of the emitter to disconnect from.
        """
        # at this point the receiver might not even be connected to the emitter yet
        # this is why we have to schedule a disconnection event and check then whether the connection exists at all
        self._circuit.remove_connection(emitter_id, self)

    # private
    def _on_value(self, signal: ValueSignal):  # virtual
        """
        Default implementation of the "value" method called whenever an upstream Emitter emitted a new Value.
        :param signal   The ValueSignal associated with this call.
        """
        pass

    def _on_failure(self, signal: FailureSignal):  # virtual
        """
        Default implementation of the "error" method called when an upstream Emitter has failed.
        By the time this function is called, the completed Emitter has already been disconnected.
        :param signal   The ErrorSignal associated with this call.
        """
        pass

    def _on_completion(self, signal: CompletionSignal):  # virtual
        """
        Default implementation of the "complete" method called when an upstream Emitter has finished successfully.
        By the time this function is called, the completed Emitter has already been disconnected.
        :param signal   The CompletionSignal associated with this call.
        """
        pass


########################################################################################################################


class Operator(Receiver, Emitter):
    """
    Operators are use-defined functions in the Circuit that take a Value and maybe produce one. Not all input values
    generate an output value though.
    Operators will complete automatically once all upstream Emitters have completed or through failure.
    """

    class CreatorHandle:
        """
        A special kind of handle that is returned when you create an Operator from a Receiver.
        It allows you to continue the chain of Operators upstream if you want to split your calculation into multiple,
        re-usable parts.
        """

        def __init__(self, operator: 'Operator'):
            """
            :param operator: Strong reference to the Operator represented by this handle.
            """
            self._operator: weak_ref = weak_ref(operator)

        def connect_to(self, emitter: Emitter.Handle):
            """
            Connects this newly created Operator to an existing Emitter upstream.
            Does nothing if the Handle has expired.
            :param emitter:     Emitter to connect to. If this Receiver is already connected, this does nothing.
            :raise TypeError:   If the Emitters's input Schema does not match.
            """
            operator: Optional[Operator] = self._operator()
            if operator is not None:
                operator.connect_to(emitter)

        def create_operator(self, operation: 'Operator.Operation', name: str = ""):
            """
            Creates and connects this newly created Operator to another new Operator upstream.
            Does nothing if the Handle has expired.
            :param operation: Operation defining the Operator.
            :param name: Name under which the Operator should be registered.
            :raise TypeError: If the output type of the Operation does not match this Receiver's.
            :raise NameError: If another Operator with the same name is already registered.
            """
            operator: Optional[Operator] = self._operator()
            if operator is not None:
                operator.create_operator(operation, name)

    class Operation(ABC):
        """
        Operations are Functors that define an Operator.
        """

        @abstractmethod
        def get_input_schema(self) -> Value.Schema:
            """
            The Schema of an input value of the Operation.
            """
            pass

        @abstractmethod
        def get_output_schema(self) -> Value.Schema:
            """
            The Schema of an output value of the Operation.
            """
            pass

        def __call__(self, value: Value) -> Optional[Value]:
            """
            Perform the Operation on a given value.
            :param value: Input value, must conform to the Schema specified by the `input_schema` property.
            :return: Either a new output value (conforming to the Schema specified by the `output_schema` property)
                or None.
            :raise TypeError: If either the input or output Value does not conform to its expected Schema.
            """
            assert value.schema == self.get_input_schema()
            result: Optional[Value] = self._perform(value)
            if result is not None and result.schema != self.get_output_schema():
                raise TypeError("Value does not conform to the Operator's input Schema")
            return result

        # private
        @abstractmethod
        def _perform(self, value: Value) -> Optional[Value]:
            """
            Operation implementation.
            If this function returns a Value, it will be emitted by the Operator.
            If this function returns None, no emission will take place.
            Exceptions thrown by this function will cause the Operator to fail.
            :param value: Input value, conforms to the input Schema.
            :return: Either a new output value conforming to the output Schema or None.
            """
            pass

    def __init__(self, circuit: 'Circuit', operation: Operation):
        """
        Constructor.
        It should only be possible to create-connect an Operator from a Receiver. 
        :param circuit: The Circuit that this Operator is a part of.
        :param operation: The Operation performed by this Operator.
        """
        Receiver.__init__(self, circuit, operation.get_input_schema())
        Emitter.__init__(self, operation.get_output_schema())

        self._operation: Operator.Operation = operation  # is constant

    # private
    def _on_value(self, signal: ValueSignal):  # final
        """
        Performs the Operation on the given Value and emits the result if one is produced.
        Exceptions thrown by the Operation will cause the Operator to fail.
        :param signal   The ValueSignal associated with this call.
        """
        try:
            result = self._operation(signal.get_value())
        except Exception as exception:
            self._fail(exception)
        else:
            if result is not None:
                self._emit(result)

    def _on_failure(self, signal: FailureSignal):  # final
        """
        If the failed Emitter was the last connected one, this Operator also completes.
        :param signal   The ErrorSignal associated with this call.
        """
        if not self.has_upstream():
            self._complete()

    def _on_completion(self, signal: CompletionSignal):  # final
        """
        If the completed Emitter was the last connected one, this Operator also completes.
        :param signal   The CompletionSignal associated with this call.
        """
        if not self.has_upstream():
            self._complete()


########################################################################################################################

class Circuit:
    """
    The Circuit object is a central object shared by all Circuit elements conceptually, even though only Receivers have
    a need for a reference to the Circuit object.
    Its main objective is to provide the handling of individual events and act as a central repository for named
    Operators.
    """

    # private
    class _Event(ABC):  # in C++ we can think of using a variant instead, as long as the number of subclasses is small

        @abstractmethod
        def handle(self) -> bool:
            """
            Handles this event.
            :returns: False on noop, if the Circuit elements have expired for example. True otherwise.
            """
            return False

    class _NoopEvent(_Event):  # this should probably be a test-only thing ...
        """
        Event used to flush the queue, useful for tests where we can have topology changes happen outside events.
        """

        def handle(self) -> bool:
            """
            Does nothing, but returns True to make sure that outstanding topology changes are applied.
            """
            return True

    class _CompletionEvent(_Event):
        """
        Event object denoting the completion of an Emitter in the Circuit.
        """

        def __init__(self, emitter: weak_ref):
            """
            Constructor.
            :param emitter: Emitter targeted by the event.
            """
            self._emitter: weak_ref = emitter

        def handle(self) -> bool:
            """
            Handles this event.
            :returns: False on noop, if the Circuit elements have expired for example. True otherwise.
            """
            emitter: Optional[Emitter] = self._get_emitter()
            if emitter is None:
                return False
            emitter._complete()
            return True

        # protected
        def _get_emitter(self) -> Optional[Emitter]:
            """
            Emitter that is targeted by the event.
            """
            return self._emitter()

    class _FailureEvent(_CompletionEvent):
        """
        Event object denoting the failure of an Emitter in the Circuit.
        """

        def __init__(self, emitter: weak_ref, error: Exception):
            """
            Constructor.
            :param emitter: Emitter targeted by the event.
            :param error:   Error causing the failure.
            """
            Circuit._CompletionEvent.__init__(self, emitter)

            self._error: Exception = error  # is constant

        def handle(self) -> bool:
            """
            Handles this event.
            :returns: False on noop, if the Circuit elements have expired for example. True otherwise.
            """
            emitter: Optional[Emitter] = self._get_emitter()
            if emitter is None:
                return False
            emitter._fail(self._error)
            return True

    class _ValueEvent(_CompletionEvent):
        """
        Event object denoting the emission of a Value from an Emitter in the Circuit.
        """

        def __init__(self, emitter: weak_ref, value: Value = Value()):
            """
            Constructor.
            :param emitter: Emitter targeted by the event.
            :param value:   Emitted Value.
            """
            # by this point, the value's schema should match the Emitter's
            assert emitter().get_output_schema() == value.schema

            Circuit._CompletionEvent.__init__(self, emitter)

            self._value: Value = value  # is constant

        def handle(self) -> bool:
            """
            Handles this event.
            :returns: False on noop, if the Circuit elements have expired for example. True otherwise.
            """
            emitter: Optional[Emitter] = self._get_emitter()
            if emitter is None:
                return False
            emitter._emit(self._value)
            return True

    class _AlreadyCompletedEvent(_Event):
        """
        Internal event created when a Receiver connects to a completed Emitter.
        Instead of having the (now completed) Emitter emit their CompletionSignal again (which they are not allowed to),
        just create a CompletionSignal out of thin air, put the completed Emitter as source and fire it to the Receiver.
        """

        def __init__(self, emitter_id: int, receiver: weak_ref):
            """
            Constructor.
            :param emitter_id:  ID of the completed Emitter.
            :param receiver:    Receiver that connected to the Emitter with the given ID.
            """
            self._emitter_id: int = emitter_id
            self._receiver: weak_ref = receiver

        def handle(self) -> bool:
            """
            Handles this event.
            :returns: False on noop, if the Circuit elements have expired for example. True otherwise.
            """
            receiver: Optional[Receiver] = self._receiver()
            if receiver is None:
                return False
            receiver.on_completion(CompletionSignal(self._emitter_id))
            return True

    class _TopologyChange:

        class Kind(Enum):
            """
            There are only two kinds of changes to a DAG: the creation of a new edge or the removal of an existing one.
            """
            CREATE_CONNECTION = 0
            REMOVE_CONNECTION = 1

        def __init__(self, emitter_id: int, receiver: weak_ref, kind: Kind):
            """
            Constructor.
            :param emitter_id:  ID of the emitter at the source of the connection
            :param receiver:    Receiver at the target of the connection.
            :param kind:        Kind of topology change.
            """
            self._emitter_id: int = emitter_id  # is constant
            self._receiver: weak_ref = receiver  # is constant
            self._kind = kind  # is constant

        def get_kind(self) -> Kind:
            """
            The kind of topology change.
            """
            return self._kind

        def get_endpoints(self) -> Optional[Tuple[Emitter, Receiver]]:
            """
            Returns a tuple (Emitter, Receiver) denoting the end-points of the Connection or None, if any one of the
            two has expired
            """
            # check if the receiver is still alive
            receiver: Optional[Receiver] = self._receiver()
            if receiver is None:
                return None

            # find the emitter
            for candidate in receiver._upstream:
                if candidate.get_id() == self._emitter_id:
                    emitter: Emitter = candidate
                    break
            else:
                return None

            # if both are still alive, return the pair
            return emitter, receiver

    # public

    def __init__(self):
        """
        Constructor.
        """
        self._events: Deque[Circuit._Event] = deque()  # main event queue
        self._event_mutex: Lock = Lock()  # mutex protecting the event queue
        self._event_condition: Condition = Condition(self._event_mutex)  # notifies watchers when a new event arrives

        self._named_emitters: Dict[str, weak_ref] = {}  # weak references to named Emitters by name
        self._name_mutex: Lock = Lock()  # mutex protecting the named emitter dictionary

        self._topology_changes: List[Circuit._TopologyChange] = []  # changes to the topology during an event in order

        self._expired_emitters: List[Emitter] = []  # all expired Emitters kept around for delayed cleanup on idle
        self._cleanup_mutex: Lock = Lock()  # mutex protecting the expired emitters list

    def emit_value(self, emitter: Emitter, value: Any = Value()):
        """
        Schedules the emission of a Value in the Circuit.
        :param emitter:     Emitter to emit from.
        :param value:       Value to emit.
        :raise TypeError:   If the Value schema does not match the Emitter's.
        """
        # implicit value conversion
        if not isinstance(value, Value):
            try:
                value = Value(value)
            except ValueError:
                raise TypeError("Emitters can only emit Values or things that are implicitly convertible to one")

        # ensure that the value can be emitted by the Emitter
        if emitter.get_output_schema() != value.schema:
            raise TypeError(f"Cannot emit Value from Emitter {emitter.get_id()}."
                            f"  Emitter schema: {emitter.get_output_schema()}"
                            f"  Value schema: {value.schema}")

        # schedule the event
        with self._event_condition:
            self._events.append(Circuit._ValueEvent(weak_ref(emitter), value))
            self._event_condition.notify_all()

    def emit_failure(self, emitter: Emitter, error: Exception):
        """
        Schedules the failure of an Emitter in the Circuit.
        :param emitter: Emitter to fail.
        :param error:   Error causing the failure.
        """
        with self._event_condition:
            self._events.append(Circuit._FailureEvent(weak_ref(emitter), error))
            self._event_condition.notify_all()

    def emit_completion(self, emitter: Emitter):
        """
        Schedules the voluntary completion of an Emitter in the Circuit.
        :param emitter: Emitter to complete.
        """
        with self._event_condition:
            self._events.append(Circuit._CompletionEvent(weak_ref(emitter)))
            self._event_condition.notify_all()

    def cleanup(self):
        """
        Deletes of all expired Emitters in the Circuit.
        """
        with self._cleanup_mutex:
            expired_emitters: List[Emitter] = self._expired_emitters
            self._expired_emitters = []
        expired_emitters.clear()

    # public for friends
    def create_emitter(self, schema: Value.Schema = Value.Schema(),
                       is_blockable: bool = False,
                       name: str = "") -> Emitter:
        """
        Creates a new Emitter in the Circuit.
        This method should only be called by classes that we have full control over (Nodes and Services?), because it
        returns a strong reference to an Emitter.
        :param schema:          Schema of the Value emitted by this Emitter, defaults to empty.
        :param is_blockable:    Whether or not Signals from this Emitter can be blocked, defaults to False.
        :param name:            (Optional) Name under which the Emitter should be registered with the Circuit.
        :raise NameError:       If another Emitter with the same name is already registered.
        """
        # create the Emitter so you can store a weak reference to it, if the user provided a name
        emitter: Emitter = Emitter(schema, is_blockable)

        # make sure the name is not already taken by another living Emitter
        if name:
            with self._name_mutex:
                existing: Optional[weak_ref] = self._named_emitters.get(name, None)
                if existing is not None and existing() is not None:
                    raise NameError(f"Failed to create Emitter {name} as another Emitter with the name already exists")
                else:
                    # register the emitter with the circuit
                    self._named_emitters[name] = weak_ref(emitter)

        return emitter

    def create_receiver(self, schema: Value.Schema = Value.Schema()) -> Receiver:
        """
        Creates a new Receiver in the Circuit.
        This method should only be called by classes that we have full control over (Nodes and Services?), because it
        returns a strong reference to a Receiver.
        :param schema:          Schema of the Value emitted by this Emitter, defaults to empty.
        """
        return Receiver(self, schema)

    def create_connection(self, emitter_id: int, receiver: Receiver):
        """
        Schedules the addition of a connection in the Circuit.
        This method should only be called by a Receiver from within the Event loop.
        :param emitter_id:  ID of the emitter at the source of the connection
        :param receiver:    Receiver at the target of the connection.
        """
        self._topology_changes.append(
            Circuit._TopologyChange(emitter_id, weak_ref(receiver), Circuit._TopologyChange.Kind.CREATE_CONNECTION))

    def remove_connection(self, emitter_id: int, receiver: Receiver):
        """
        Schedules the removal of a connection in the Circuit.
        This method should only be called by a Receiver from within the Event loop.
        :param emitter_id:  ID of the emitter at the source of the connection
        :param receiver:    Receiver at the target of the connection.
        """
        self._topology_changes.append(
            Circuit._TopologyChange(emitter_id, weak_ref(receiver), Circuit._TopologyChange.Kind.REMOVE_CONNECTION))

    def expire_emitter(self, emitter: Emitter):
        """
        Passes ownership of a (potentially) expired Emitter to the Circuit so it can be deleted when the application is
        idle. If there are other owners of the Emitter, this does nothing.
        :param emitter: Emitter to expire (in C++ this would be an r-value).
        """
        with self._cleanup_mutex:
            self._expired_emitters.append(emitter)

    def await_event(self, timeout: Optional[float] = None) -> Optional[_Event]:
        """
        Blocks the calling thread until a time where at least one new event has been queued in the Circuit.
        Should only be called by the EventLoop.
        :param timeout: Optional timeout in seconds.
        """
        with self._event_condition:
            if not self._event_condition.wait_for(lambda: len(self._events) > 0, timeout):
                return None
            return self._events.popleft()

    def handle_event(self, event: 'Circuit._Event'):
        """
        Handles the next event, if one exists.
        Should only be called by the EventLoop.
        :param event: Event to handle.
        """
        # at this point, we should have no outstanding topology changes to perform (unless during testing)
        if not isinstance(event, self._NoopEvent):
            assert len(self._topology_changes) == 0

        # handle the event
        if not event.handle():
            return

        # perform all delayed changes to the topology of the circuit
        for change in self._topology_changes:
            # the affected topology might not even exist anymore
            connection: Optional[Tuple[Emitter, Receiver]] = change.get_endpoints()
            if connection is None:
                continue

            # perform the topology change
            if change.get_kind() == Circuit._TopologyChange.Kind.CREATE_CONNECTION:
                self._create_connection(*connection)
            elif change.get_kind() == Circuit._TopologyChange.Kind.REMOVE_CONNECTION:
                self._remove_connection(*connection)
            else:
                assert False

        self._topology_changes.clear()

    # private

    def _create_connection(self, emitter: Emitter, receiver: Receiver):
        """
        Creates a new connection in the Circuit during the event epilogue.
        :param emitter: Emitter at the source of the connection.
        :param receiver: Receiver at the target of the connection.
        """
        # never create a connection mid-event
        assert emitter._status in (Emitter._Status.IDLE, Emitter._Status.COMPLETED)

        # multiple connections between the same Emitter-Receiver pair are ignored
        weak_receiver = weak_ref(receiver)
        if weak_receiver in emitter._downstream:  # can only be true if the emitter has not completed
            return

        # It is possible that the Receiver already has a strong reference to the Emitter if it just created it.
        receiver._upstream.add(emitter)

        # if the Emitter has already completed, schedule a CompletionSignal as a continuation of this Event.
        if emitter.is_completed():
            with self._event_condition:
                self._events.append(Circuit._AlreadyCompletedEvent(emitter.get_id(), weak_receiver))
                self._event_condition.notify_all()

        # if however the Emitter is still alive, add the Receiver as a downstream
        else:
            emitter._downstream.append(weak_receiver)

    def _remove_connection(self, emitter: Emitter, receiver: Receiver):
        """
        Removes a connection in the Circuit during the event epilogue.
        :param emitter: Emitter at the source of the connection.
        :param receiver: Receiver at the target of the connection.
        """
        # remove the connection by removing the mutual references
        receiver._upstream.remove(emitter)
        emitter._downstream.remove(weak_ref(receiver))

        # mark the emitter as ready for deletion
        self.expire_emitter(emitter)
