from abc import ABC, abstractmethod
from collections import deque
from enum import Enum
from logging import error as print_error
from threading import Lock, Condition
from typing import Any, Optional, List, Tuple, Set, Deque, Dict, Callable, NamedTuple, NewType, TypeVar, Type
from weakref import ref as weak_ref

from .value import Value

T = TypeVar('T')


########################################################################################################################

class Error(NamedTuple):
    """
    Object wrapping any exception thrown (and caught) in the Circuit alongside additional information.
    """

    class Kind(Enum):
        NO_DAG = 1  # Exception raised when a cyclic dependency was detected in the graph.
        WRONG_VALUE_SCHEMA = 2
        USER_CODE_EXCEPTION = 3

    element: weak_ref  # weak reference to the element
    kind: Kind  # kind of error caught in the circuit
    message: str  # error message


########################################################################################################################

ID = NewType('ID', int)
"""
The Emitter ID is a 64bit wide, ever-increasing unsigned integer that is unique in the Circuit.
"""


class Element(ABC):
    """
    Base class for all Circuit Elements.
    This is an empty abstract base class that is inherited by the AbstractEmitter and AbstractReceiver interfaces.
    Only concrete classes (that are no longer derived from) should implement the abstract Element methods.
    """

    ID = ID

    @abstractmethod
    def get_id(self) -> 'Element.ID':
        """
        The Circuit-unique identification number of this Element.
        """
        raise NotImplementedError()

    @abstractmethod
    def get_circuit(self) -> 'Circuit':
        """
        The Circuit containing this Element.
        """
        raise NotImplementedError()


########################################################################################################################

class CompletionSignal:
    """
    Signal emitted by an Emitter that produced the last value in its data stream.
    This is the last Signal to be emitted by that Emitter.
    """

    def __init__(self, emitter_id: Element.ID):
        """
        Constructor.
        :param emitter_id:      ID of the Emitter that emitted the Signal.
        """
        self._emitter_id: Element.ID = emitter_id  # is constant

    def get_source(self) -> Element.ID:
        """
        ID of the Emitter that emitted the Signal.
        """
        return self._emitter_id


########################################################################################################################

class FailureSignal(CompletionSignal):  # public inheritance in C++ ... failure is-a kind of completion
    """
    Signal emitted by an Emitter that failed to produce the next value in its data stream.
    This is the last Signal to be emitted by that Emitter.
    """

    def __init__(self, emitter_id: Element.ID, error: Exception):
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

class ValueSignal(CompletionSignal):  # protected inheritance only for the get_source function but no is-a relationship
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

    def __init__(self, emitter_id: Element.ID, value: Value, is_blockable: bool = False):
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
# noinspection PyAbstractClass
class AbstractEmitter(Element):
    """
    Virtual interface class for Circuit Elements that emit Signals.
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

        def __init__(self, emitter: 'AbstractEmitter'):
            """
            Constructor.
            :param emitter: Strong reference to the Emitter represented by this handle.
            """
            self._element: weak_ref = weak_ref(emitter)

        def is_valid(self) -> bool:
            """
            Checks if the handle is still valid.
            """
            return self._element() is not None

        def get_id(self) -> Optional[Element.ID]:
            """
            Returns the Element ID of the Emitter or None if the handle has expired.
            """
            emitter: AbstractEmitter = self._element()
            if emitter is None:
                return None
            else:
                return emitter.get_id()

        def get_output_schema(self) -> Optional[Value.Schema]:
            """
            Returns the output schema of the Emitter or None, if the handle has expired.
            """
            emitter: AbstractEmitter = self._element()
            if emitter is None:
                return None
            else:
                return emitter.get_output_schema()

    class _Status(Enum):
        """
        Emitters start out IDLE and change to EMITTING whenever they are emitting a ValueSignal to connected Receivers.
        If an Emitter tries to emit but is already in EMITTING state, we know that we have caught a cyclic dependency.
        To emit a FailureSignal or a CompletionSignal, the Emitter will switch to the respective FAILING or COMPLETING
        state. Once it has finished its `_fail` or `_complete` methods, it will permanently change its state to
        COMPLETED or FAILED and will not emit anything again.

            --> IDLE <-> EMITTING
                  |
                  +--> FAILING --> FAILED
                  |
                  +--> COMPLETING --> COMPLETE
        """
        IDLE = 0
        EMITTING = 1
        FAILING = 2
        FAILED = 3
        COMPLETING = 4
        COMPLETED = 5

    def __init__(self, schema: Value.Schema = Value.Schema(), is_blockable: bool = False):
        """
        Constructor.
        Should only be called by `Circuit.create_emitter` - directly or indirectly through subclasses.
        :param schema:          Schema of the Value emitted by this Emitter, defaults to empty.
        :param is_blockable:    Whether or not Signals from this Emitter can be blocked, defaults to False.
        """
        self._downstream: List[weak_ref] = []  # subclasses may only change the order
        self._output_schema: Value.Schema = schema  # is constant
        self._is_blockable: bool = is_blockable  # is constant
        self._status: AbstractEmitter._Status = self._Status.IDLE

    def get_output_schema(self) -> Value.Schema:  # noexcept
        """
        Schema of the emitted value. Can be the empty Schema if this Emitter does not emit a meaningful value.
        """
        return self._output_schema

    def is_blockable(self) -> bool:  # noexcept
        """
        Whether Signals emitted by this Emitter are blockable or not.
        """
        return self._is_blockable

    def is_completed(self) -> bool:  # noexcept
        """
        Returns true iff the Emitter has been completed, either through an error or normally.
        """
        return not (self._status == self._Status.IDLE or self._status == self._Status.EMITTING)

    def has_downstream(self) -> bool:  # noexcept
        """
        Checks if emitting a Value through this Emitter reaches any Receivers downstream.
        This way you can skip expensive calculations of Values if there is nobody to receive them anyway.
        """
        return len(self._downstream) > 0

    # protected
    def _emit(self, value: Any = Value()):  # noexcept
        """
        Push the given value to all active Receivers.

        :param value:       Value to emit, can be empty if this Emitter does not emit a meaningful value.
        """
        assert value.schema == self.get_output_schema()

        # make sure we can never emit once the Emitter has completed
        if self.is_completed():
            return

        # early out if there wouldn't be a point of emitting anyway
        if not self.has_downstream():
            return

        # make sure that we are not already emitting
        if self._status != self._Status.IDLE:
            return self.get_circuit().handle_error(Error(
                weak_ref(self),
                Error.Kind.NO_DAG,
                f"Cyclic dependency detected during emission from Emitter {self.get_id()}."))
        self._status = self._Status.EMITTING

        try:
            # sort out invalid receivers and compile a list of strong valid references
            receivers: List[AbstractReceiver] = []
            highest_valid_index: int = 0
            for weak_receiver in self._downstream:
                receiver: Optional[AbstractReceiver] = weak_receiver()
                if receiver is not None:
                    receivers.append(receiver)
                    self._downstream[highest_valid_index] = weak_receiver  # could be a move or swap in C++
                    highest_valid_index += 1

            # remove all dead receivers
            self._downstream = self._downstream[:highest_valid_index]

            # create the signal to emit
            signal = ValueSignal(self.get_id(), value, self.is_blockable())  # in C++ the value would be moved

            # emit to all live receivers in order
            for receiver in receivers:
                # highly unlikely, but can happen if there is a cycle in the circuit that caused this emitter to
                # complete while it is in the process of emitting a value
                if self.is_completed():
                    return

                receiver.on_value(signal)

                # stop the emission if the Signal was blocked
                # C++ will most likely be able to move this condition out of the loop
                if self.is_blockable() and signal.is_blocked():
                    break

        finally:
            # reset the emission flag if we are still emitting, his would be the job of a RAII guard in C++
            if self._status == self._Status.EMITTING:
                self._status = self._Status.IDLE

    def _fail(self, error: Exception):  # noexcept
        """
        Failure method, completes the Emitter while letting the downstream Receivers inspect the error.
        :param error:       The error that has occurred.
        """
        # make sure we can never emit once the Emitter has completed
        if self.is_completed():
            return

        # we don't have to test for cyclic dependency errors here because this method will complete the emitter
        self._status = self._Status.FAILING

        try:
            # create the signal
            signal: FailureSignal = FailureSignal(self.get_id(), error)

            # emit to all receivers, ignore expired ones but don't remove them
            for weak_receiver in self._downstream:
                receiver: Optional[AbstractReceiver] = weak_receiver()
                if receiver is None:
                    continue

                receiver.on_failure(signal)

        finally:
            # complete the emitter
            self._downstream.clear()
            self._status = self._Status.FAILED

    def _complete(self):  # noexcept
        """
        Completes the Emitter successfully.
        """
        # make sure we can never emit once the Emitter has completed
        if self.is_completed():
            return

        # we don't have to test for cyclic dependency errors here because this method will complete the emitter
        self._status = self._Status.COMPLETING

        try:
            # create the signal
            signal: CompletionSignal = CompletionSignal(self.get_id())

            # emit to all receivers, ignore expired ones but don't remove them
            for weak_receiver in self._downstream:
                receiver: Optional[AbstractReceiver] = weak_receiver()
                if receiver is None:
                    continue

                receiver.on_completion(signal)

        finally:
            # complete the emitter
            self._downstream.clear()
            self._status = self._Status.COMPLETED


########################################################################################################################
# noinspection PyAbstractClass
class AbstractReceiver(Element):
    """
    Virtual interface class for Circuit Elements that receive Signals.
    Is non-copyable and may only be owned by other Receivers downstream and Nodes.
    """

    def __init__(self, schema: Value.Schema = Value.Schema()):  # noexcept
        """
        Constructor.
        Should only be called by `Circuit.create_receiver` and the Operator constructor.
        :param schema:  Schema defining the Value expected by this Receiver, defaults to empty.
        """
        self._input_schema: Value.Schema = schema  # is constant
        self._upstream: Set[AbstractEmitter] = set()

    def get_input_schema(self) -> Value.Schema:  # noexcept
        """
        Schema of expected values. Can be the empty Schema.
        """
        return self._input_schema

    def has_upstream(self) -> bool:  # noexcept
        """
        Checks if the Receiver has any Emitter connected upstream.
        Useful for overrides of `_on_failure` and `_on_complete` to find out whether the completed Emitter was the last
        connected one.
        """
        return len(self._upstream) != 0

    def create_operator(self, operation: 'Operator.Operation') -> Optional['Operator.CreatorHandle']:  # noexcept
        """
        Creates and connects to a new Operator upstream.
        :param operation: Operation defining the Operator.
        :returns: New Operator or None if the output type of the Operation does not match this Receiver's.
        """
        # fail immediately if the schemas don't match
        if self.get_input_schema() != operation.get_output_schema():
            return self.get_circuit().handle_error(Error(
                weak_ref(self),
                Error.Kind.WRONG_VALUE_SCHEMA,
                f"Cannot create an Operator with the output Value Schema {operation.get_output_schema()} "
                f"from a Receiver with the input Value Schema {self.get_input_schema()}"))

        # create the Operator
        operator: Operator = self.get_circuit().create_element(Operator, operation)

        # store the operator right away to keep it alive, this does not count as a connection yet
        self._upstream.add(operator)

        # the actual connection is delayed until the end of the event
        self.get_circuit().create_connection(operator, self)

        # return a handle to the freshly created Operator
        return Operator.CreatorHandle(operator)

    def connect_to(self, emitter_handle: AbstractEmitter.Handle):  # noexcept
        """
        Connect to the given Emitter.
        If the Emitter is invalid or this Receiver is already connected to it, this does nothing.
        :param emitter_handle: Handle of the Emitter to connect to.
        """
        # get the emitter to connect to, if it is still alive
        emitter: Optional[AbstractEmitter] = emitter_handle._element()
        if emitter is None:
            return

        # fail immediately if the schemas don't match
        if self.get_input_schema() != emitter.get_output_schema():
            return self.get_circuit().handle_error(Error(
                weak_ref(self),
                Error.Kind.WRONG_VALUE_SCHEMA,
                f"Cannot connect Emitter {emitter.get_id()} "
                f"with the output Value Schema {emitter.get_output_schema()} "
                f"to a Receiver with the input Value Schema {self.get_input_schema()}"))

        # store the operator right away to keep it alive
        # as the emitter does not know about this, it does not count as a connection yet
        self._upstream.add(emitter)

        # schedule the creation of the connection
        self.get_circuit().create_connection(emitter, self)

    def disconnect_from(self, emitter_handle: AbstractEmitter.Handle):
        """
        Disconnects from the given Emitter.
        :param emitter_handle:  Handle of the emitter to disconnect from.
        """
        # get the emitter to disconnect from, if it is still alive
        emitter: Optional[AbstractEmitter] = emitter_handle._element()
        if emitter is None:
            return

        # At this point the receiver might not even be connected to the emitter yet, if the connection was created as
        # part of the same event as this call to disconnect.
        # This is why we have to schedule a disconnection event and check then whether the connection exists at all.
        self.get_circuit().remove_connection(emitter, self)

    def disconnect_upstream(self):
        """
        Removes all upstream connections of this Receiver.
        """
        for emitter in self._upstream:
            self.get_circuit().remove_connection(emitter, self)

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
        try:
            self._on_value(signal)
        except Exception as error:
            self.get_circuit().handle_error(Error(weak_ref(self), Error.Kind.USER_CODE_EXCEPTION, str(error)))

    def on_failure(self, signal: FailureSignal):
        """
        Called when an upstream Emitter has failed.
        :param signal       The ErrorSignal associated with this call.
        """
        # assert that the emitter is connected to this receiver
        for candidate in self._upstream:
            if candidate.get_id() == signal.get_source():
                emitter: AbstractEmitter = candidate
                break
        else:
            assert False  # received Signal from an Emitter that is not connected

        # since the emitter has already completed, there is no reason to delay disconnecting
        # it will never emit again anyway
        self._upstream.remove(emitter)

        # mark the emitter as a candidate for deletion
        self.get_circuit().expire_emitter(emitter)

        # pass the signal along to the user-defined handler
        try:
            self._on_failure(signal)
        except Exception as error:
            self.get_circuit().handle_error(Error(weak_ref(self), Error.Kind.USER_CODE_EXCEPTION, str(error)))

    def on_completion(self, signal: CompletionSignal):
        """
        Called when an upstream Emitter has finished successfully.
        :param signal       The CompletionSignal associated with this call.
        """
        # assert that the emitter is connected to this receiver
        for candidate in self._upstream:
            if candidate.get_id() == signal.get_source():
                emitter: AbstractEmitter = candidate
                break
        else:
            assert False  # received Signal from an Emitter that is not connected

        # since the emitter has already completed, there is no reason to delay disconnecting
        # it will never emit again anyway
        self._upstream.remove(emitter)

        # mark the emitter as ready for deletion
        self.get_circuit().expire_emitter(emitter)

        # pass the signal along to the user-defined handler
        try:
            self._on_completion(signal)
        except Exception as error:
            self.get_circuit().handle_error(Error(weak_ref(self), Error.Kind.USER_CODE_EXCEPTION, str(error)))

    # private
    def _find_emitter(self, emitter_id: AbstractEmitter.ID) -> Optional[AbstractEmitter.Handle]:
        """
        Finds and returns an Emitter.Handle of any Emitter in the Circuit.
        :param emitter_id:  ID of the Emitter to find.
        :return:            A (non-owning) handle to the requested Emitter or NOne.
        """
        element: Optional[Element] = self.get_circuit().get_element(emitter_id)
        if element is None or not isinstance(element, AbstractEmitter):
            return None
        return AbstractEmitter.Handle(element)

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


class Operator(AbstractReceiver, AbstractEmitter):  # final
    """
    Operators are use-defined functions in the Circuit that take a Value and maybe produce one. Not all input values
    generate an output value though.
    Operators will complete automatically once all upstream Emitters have completed or through failure.
    """

    class CreatorHandle(AbstractEmitter.Handle):
        """
        A special kind of handle that is returned when you create an Operator from a Receiver.
        It allows you to continue the chain of Operators upstream if you want to split your calculation into multiple,
        re-usable parts.
        """

        def __init__(self, operator: 'Operator'):
            """
            :param operator: Strong reference to the Operator represented by this handle.
            """
            AbstractEmitter.Handle.__init__(self, operator)

        def connect_to(self, emitter_handle: AbstractEmitter.Handle):
            """
            Connects this newly created Operator to an existing Emitter upstream.
            Does nothing if the Handle has expired.
            :param emitter_handle:  Emitter to connect to. If this Receiver is already connected, this does nothing.
            """
            operator: Optional[Operator] = self._element()
            if operator is not None:
                assert isinstance(operator, Operator)
                operator.connect_to(emitter_handle)

        def create_operator(self, operation: 'Operator.Operation') -> Optional['Operator.CreatorHandle']:  # noexcept
            """
            Creates and connects this newly created Operator to another new Operator upstream.
            Does nothing if the Handle has expired.
            :param operation: Operation defining the Operator.
            """
            operator: Optional[Operator] = self._element()
            if operator is not None:
                assert isinstance(operator, Operator)
                return operator.create_operator(operation)

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

        @abstractmethod
        def __call__(self, value: Value) -> Optional[Value]:
            """
            Perform the Operation on a given value.
            If this function returns a Value, it will be emitted by the Operator.
            If this function returns None, no emission will take place.
            Exceptions thrown by this function will cause the Operator to fail.
            :param value: Input value, conforms to the input Schema.
            :return: Either a new output value conforming to the output Schema or None.
            """
            raise NotImplementedError()

    def __init__(self, circuit: 'Circuit', element_id: Element.ID, operation: Operation):
        """
        Constructor.
        It should only be possible to create-connect an Operator from a Receiver. 
        :param circuit:     The Circuit containing this Element.
        :param element_id:  The Circuit-unique identification number of this Element.
        :param operation:   The Operation performed by this Operator.
        """
        AbstractReceiver.__init__(self, operation.get_input_schema())
        AbstractEmitter.__init__(self, operation.get_output_schema())

        self._circuit: Circuit = circuit  # is constant
        self._element_id: Element.ID = element_id  # is constant
        self._operation: Operator.Operation = operation  # is constant

    def get_id(self) -> Element.ID:  # final, noexcept
        """
        The Circuit-unique identification number of this Element.
        """
        return self._element_id

    def get_circuit(self) -> 'Circuit':  # final, noexcept
        """
        The Circuit containing this Element.
        """
        return self._circuit

    # private
    def _on_value(self, signal: ValueSignal):  # final
        """
        Performs the Operation on the given Value and emits the result if one is produced.
        Exceptions thrown by the Operation will cause the Operator to fail.
        :param signal   The ValueSignal associated with this call.
        """
        value: Value = signal.get_value()
        assert value.schema == self.get_input_schema()

        # invoke the operation to produce a result or an error
        error: Optional[Exception] = None
        result: Optional[Value] = None
        try:
            result = self._operation(value)
        except Exception as exception:
            error = exception
        else:
            # if no error occurred, but no result was produced either, there's nothing left to do
            if result is None:
                return
            elif result.schema != self.get_output_schema():
                error = TypeError("Return Value of Operation does not conform to the Operator's output Schema")

        # if any error occurred, the Operator will complete through failure
        if error is not None:
            self.get_circuit().handle_error(Error(weak_ref(self), Error.Kind.USER_CODE_EXCEPTION, str(error)))
            self._fail(error)

        # if a valid result was produced successfully, emit it
        else:
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

    Error = Error

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
            emitter: Optional[AbstractEmitter] = self._get_emitter()
            if emitter is None:
                return False
            emitter._complete()
            return True

        # protected
        def _get_emitter(self) -> Optional[AbstractEmitter]:
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
            emitter: Optional[AbstractEmitter] = self._get_emitter()
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
            emitter: Optional[AbstractEmitter] = self._get_emitter()
            if emitter is None:
                return False
            emitter._emit(self._value)
            return True

    class _AlreadyCompletedEvent(_Event):
        """
        Internal event created when a Receiver connects to a completed Emitter.
        Instead of having the (now completed) Emitter emit their CompletionSignal again (which they are not allowed to),
        just create a CompletionSignal out of thin air, put the completed Emitter as source and hand it to the Receiver.
        """

        def __init__(self, emitter_id: Element.ID, receiver: weak_ref):
            """
            Constructor.
            :param emitter_id:  ID of the completed Emitter.
            :param receiver:    Receiver that connected to the Emitter with the given ID.
            """
            self._emitter_id: Element.ID = emitter_id
            self._receiver: weak_ref = receiver

        def handle(self) -> bool:
            """
            Handles this event.
            :returns: False on noop, if the Circuit elements have expired for example. True otherwise.
            """
            receiver: Optional[AbstractReceiver] = self._receiver()
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

        def __init__(self, emitter: weak_ref, receiver: weak_ref, kind: Kind):
            """
            Constructor.
            :param emitter:     Emitter at the source of the connection.
            :param receiver:    Receiver at the target of the connection.
            :param kind:        Kind of topology change.
            """
            self._emitter: weak_ref = emitter  # is constant
            self._receiver: weak_ref = receiver  # is constant
            self._kind = kind  # is constant

        def get_kind(self) -> Kind:
            """
            The kind of topology change.
            """
            return self._kind

        def get_endpoints(self) -> Optional[Tuple[AbstractEmitter, AbstractReceiver]]:
            """
            Returns a tuple (Emitter, Receiver) denoting the end-points of the Connection or None, if any one of the
            two has expired
            """
            # check if the receiver is still alive
            receiver: Optional[AbstractReceiver] = self._receiver()
            if receiver is None:
                return None

            # check if the emitter is still alive
            emitter: Optional[AbstractEmitter] = self._emitter()
            if emitter is None:
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

        self._elements: Dict[Element.ID, weak_ref] = {}  # all elements in the Circuit by their ID
        self._element_mutex: Lock = Lock()
        self._next_id: int = 1  # next available element ID

        # these members should not require a mutex
        self._topology_changes: List[Circuit._TopologyChange] = []  # changes to the topology during an event in order
        self._expired_elements: List[Element] = []  # all expired Elements kept around for delayed cleanup on idle
        self._error_callback: Optional[Callable] = None

    def emit_value(self, emitter: AbstractEmitter, value: Any = Value()):
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

    def emit_failure(self, emitter: AbstractEmitter, error: Exception):
        """
        Schedules the failure of an Emitter in the Circuit.
        :param emitter: Emitter to fail.
        :param error:   Error causing the failure.
        """
        with self._event_condition:
            self._events.append(Circuit._FailureEvent(weak_ref(emitter), error))
            self._event_condition.notify_all()

    def emit_completion(self, emitter: AbstractEmitter):
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
        self._expired_elements.clear()

    # public for friends
    def create_element(self, element_type: Type[T], *args, **kwargs) -> T:
        assert issubclass(element_type, Element)

        # always increase the counter even if the Element hasn't been built yet
        with self._element_mutex:
            element_id: Element.ID = Element.ID(self._next_id)
            self._next_id += 1  # in C++ this could be an atomic

        # create the new element
        element: T = element_type(self, element_id, *args, **kwargs)

        # register the new element with the circuit
        with self._element_mutex:
            self._elements[element_id] = weak_ref(element)

        return element

    def get_element(self, element_id: Element.ID) -> Optional[Element]:
        """
        Queries a Circuit Element by ID.
        :param element_id:  ID of the requested Element.
        :return:            The requested Element or None if no live Element exists with the given ID.
        """
        with self._element_mutex:
            weak_element: Optional[weak_ref] = self._elements.get(element_id, None)
            if weak_element is None:
                return None
            element: Optional[Element] = weak_element()
            if element is None:
                del self._elements[element_id]
            return element

    def create_connection(self, emitter: AbstractEmitter, receiver: AbstractReceiver):
        """
        Schedules the addition of a connection in the Circuit.
        This method should only be called by a Receiver from within the Event loop.
        :param emitter:     Emitter at the source of the connection.
        :param receiver:    Receiver at the target of the connection.
        """
        self._topology_changes.append(Circuit._TopologyChange(weak_ref(emitter), weak_ref(receiver),
                                                              Circuit._TopologyChange.Kind.CREATE_CONNECTION))

    def remove_connection(self, emitter: AbstractEmitter, receiver: AbstractReceiver):
        """
        Schedules the removal of a connection in the Circuit.
        This method should only be called by a Receiver from within the Event loop.
        :param emitter:     Emitter at the source of the connection.
        :param receiver:    Receiver at the target of the connection.
        """
        self._topology_changes.append(Circuit._TopologyChange(weak_ref(emitter), weak_ref(receiver),
                                                              Circuit._TopologyChange.Kind.REMOVE_CONNECTION))

    def expire_emitter(self, emitter: AbstractEmitter):
        """
        Passes ownership of a (potentially) expired Emitter to the Circuit so it can be deleted when the application is
        idle. If there are other owners of the Emitter, this does nothing.
        :param emitter: Emitter to expire (in C++ this would be an r-value).
        """
        self._expired_elements.append(emitter)

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
            connection: Optional[Tuple[AbstractEmitter, AbstractReceiver]] = change.get_endpoints()
            if connection is None:
                continue

            # perform the topology change
            if change.get_kind() == Circuit._TopologyChange.Kind.CREATE_CONNECTION:
                self._create_connection(*connection)
            else:
                assert change.get_kind() == Circuit._TopologyChange.Kind.REMOVE_CONNECTION
                self._remove_connection(*connection)

        self._topology_changes.clear()

    def set_error_callback(self, callback: Optional[Callable]):
        """
        Defines the function called whenever an error is caught in the Circuit.
        :param callback:    Error callback, passing None will cause the Circuit to use the default error handler.
        """
        self._error_callback = callback

    def handle_error(self, error: Error):
        """
        :param error: Error caught in the Circuit.
        """
        # if the user installed an error handler, use that to report the error
        if self._error_callback is not None:
            self._error_callback(error)

        # otherwise just print an error message to stderr and hope for the best
        else:
            print_error(f"Circuit failed during Event handling.\n"
                        f"{error.message}")

    # private

    def _create_connection(self, emitter: AbstractEmitter, receiver: AbstractReceiver):
        """
        Creates a new connection in the Circuit during the event epilogue.
        :param emitter: Emitter at the source of the connection.
        :param receiver: Receiver at the target of the connection.
        """
        # never create a connection mid-event
        assert emitter._status in (AbstractEmitter._Status.IDLE,
                                   AbstractEmitter._Status.FAILED,
                                   AbstractEmitter._Status.COMPLETED)

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

    def _remove_connection(self, emitter: AbstractEmitter, receiver: AbstractReceiver):
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
