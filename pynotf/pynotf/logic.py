from abc import ABCMeta, abstractmethod
from enum import Enum
from inspect import signature
from logging import error as print_error
from traceback import format_exc
from typing import Any, Optional, List, Callable, Tuple
from weakref import ref as weak_ref

from .value import Value


########################################################################################################################

class ValueSignal:
    class Status(Enum):
        """
        Status of the Signal with the following state transition diagram:
            --> Ignored --> Accepted --> Blocked
                    |                       ^
                    +-----------------------+
            --> Unblockable
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
        self._emitter_id: int = emitter_id  # Is constant
        self._value: Value = value  # Is constant
        self._status: ValueSignal.Status = self.Status.UNHANDLED if is_blockable else self.Status.UNBLOCKABLE

    def get_source(self) -> int:
        """
        ID of the Emitter that emitted the Signal.
        """
        return self._emitter_id

    def get_value(self) -> Value:
        """
        Immutable Value contained in the Signal.
        """
        return self._value

    def is_blockable(self) -> bool:
        """
        Whether or not the Signal was at some point able to be accepted and blocked by Receivers.
        """
        return self._status != self.Status.UNBLOCKABLE

    def is_accepted(self) -> bool:
        """
        Returns true iff this Signal is blockable and has been accepted or blocked.
        """
        return self._status in (self.Status.ACCEPTED, self.Status.BLOCKED)

    def is_blocked(self) -> bool:
        """
        Returns true iff this Signal is blockable and has been blocked.
        """
        return self._status == self.Status.BLOCKED

    def accept(self):
        """
        If this Signal is blockable and has not been accepted yet, mark it as accepted.
        """
        if self._status == self.Status.UNHANDLED:
            self._status = self.Status.ACCEPTED

    def block(self):
        """
        If this Signal is blockable and has not been blocked yet, mark it as blocked.
        """
        if self._status in (self.Status.UNHANDLED, self.Status.ACCEPTED):
            self._status = self.Status.BLOCKED


########################################################################################################################

class ErrorSignal:
    def __init__(self, emitter_id: int, error: Exception):
        """
        Constructor.
        :param emitter_id:      ID of the Emitter that emitted the Signal.
        :param error:           Exception produced by the Emitter.
        """
        self._emitter_id: int = emitter_id  # Is constant
        self._error: Exception = error

    def get_source(self) -> int:
        """
        ID of the Emitter that emitted the Signal.
        """
        return self._emitter_id

    def get_error(self) -> Exception:
        """
        The exception produced by the Emitter.
        """
        return self._error


########################################################################################################################

class CompletionSignal:
    def __init__(self, emitter_id: int):
        """
        Constructor.
        :param emitter_id:      ID of the Emitter that emitted the Signal.
        """
        self._emitter_id: int = emitter_id  # Is constant

    def get_source(self) -> int:
        """
        ID of the Emitter that emitted the Signal.
        """
        return self._emitter_id


########################################################################################################################

class Emitter:
    """
    Circuit element pushing a Signal downstream.
    Is non-copyable and may only be owned by downstream Receivers and Nodes.
    """

    def __init__(self, schema: Value.Schema = Value.Schema()):
        """
        Constructor.
        :param schema:  Schema of the Value emitted by this Emitter, defaults to empty.
        """
        self._downstream: List[weak_ref] = []  # Receivers are ordered but unique
        self._is_completed: bool = False  # Once a Emitter is completed, it can no longer emit any values
        self._output_schema: Value.Schema = schema  # Is constant
        self._error_handler: Callable = self._default_error_handler  # Cannot be empty

    def get_output_schema(self) -> Value.Schema:
        """
        Schema of the emitted value. Can be the empty Schema if this Emitter does not emit a meaningful value.
        """
        return self._output_schema

    def get_id(self) -> int:
        """
        Unique identifier of this Circuit element.
        """
        return id(self)

    def is_completed(self) -> bool:
        """
        Returns true iff the Emitter has been completed, either through an error or normally.
        """
        return self._is_completed

    def set_error_handler(self, error_handler: Optional[Callable]):
        """
        Installs a new error handler callback on this Emitter.
        Error handlers are functions with the signature:
             handler(receiver: Receiver, exception: Exception) -> None
        :param error_handler:   Error handler to install. If empty, the default handler will be used.
        :raise TypeError:       If the given handler does not match the signature.
        """
        # you can always drop back to the default by passing None
        if error_handler is None:
            self._error_handler = self._default_error_handler

        else:
            # new handlers must have the correct signature
            if signature(error_handler).parameters != signature(self._default_error_handler).parameters:
                raise TypeError("The signature of an error handler must match the default error handler's")
            self._error_handler = error_handler

    def emit(self, value: Any = Value()):
        """
        Push the given value to all active Receivers.
        :param value: Value to emit, can be empty if this Emitter does not emit a meaningful value.
        :raise TypeError: If the Value's Schema doesn't match.
        :raise RuntimeError: If the Emitter has already completed (either normally or though an error).
        """
        # First, the Emitter needs to check if it is already completed. No point doing anything, if the Emitter is
        # unable to emit anything. This should never happen, so raise an exception if it does.
        if self.is_completed():
            raise RuntimeError("Cannot emit from a completed Emitter")

        # Check the given value to see if the Emitter is allowed to emit it. If it is not a Value, try to
        # build one out of it. If that doesn't work, or the Schema of the Value does not match that of the
        # Emitter, raise an exception.
        if not isinstance(value, Value):
            try:
                value = Value(value)
            except ValueError:
                raise TypeError("Emitter can only emit values that are implicitly convertible to a Value")
        if value.schema != self.get_output_schema():
            raise TypeError("Emitter cannot emit a value with the wrong Schema")

        # Emit from the local list of strong references that we know will stay alive. The member field may change
        # during the iteration because some Receiver downstream might add to the list of Receivers or even disconnect
        # other Receivers, but those changes will not affect the current emitting process.
        self._emit(self._collect_receivers(), value)

    # protected
    def _error(self, exception: Exception):
        """
        Failure method, completes the Emitter.
        This method would be protected in C++. It is not private, but should not be part of the public interface.
        :param exception:   The exception that has occurred.
        """
        print_error(format_exc())

        signal: ErrorSignal = ErrorSignal(self.get_id(), exception)
        for receiver in self._collect_receivers():
            try:
                receiver.on_error(signal)
            except Exception:
                print_error(format_exc())

        self._downstream.clear()
        self._is_completed = True

    def _complete(self):
        """
        Completes the Emitter successfully.
        This method would be protected in C++. It is not private, but should not be part of the public interface.
        """
        signal: CompletionSignal = CompletionSignal(self.get_id())
        for receiver in self._collect_receivers():
            try:
                receiver.on_complete(signal)
            except Exception:
                print_error(format_exc())

        self._downstream.clear()
        self._is_completed = True

    def _sort_receivers(self, order: List[int]):
        """
        Changes the order of the Receivers of this Emitter without giving write access to the `_downstream` field.
        Example: Given Receivers [a, b, c], then calling `self._sort_receivers([2, 0, 1])` will change the order
        to [c, a, b]. Invalid orders will raise an exception.
        :param order: New order of the receivers.
        :raise ValueError: If the given order is invalid.
        """
        if sorted(order) != list(range(len(self._downstream))):
            raise ValueError(f"Invalid order: {order} for a Emitter with {len(self._downstream)} Receivers")
        self._downstream = [self._downstream[index] for index in order]

    # private
    def _emit(self, receivers: List['Receiver'], value: Value):
        """
        This method can be overwritten in subclasses to modify the emitting process.
        At the point where this method is called, we have established that `value` can be emitted by this Emitter,
        meaning its Schema matches the output Schema of this Emitter and the Emitter has not been completed. All
        expired Receivers have been removed from the list of Receivers and the `receivers` argument is a list of non-
        owning (but valid) references matching the `self._downstream` weak list.
        The default implementation of this method simply emits an unblockable Signal to every Receiver in order.
        Subclasses can do further sorting and if they choose to, can re-apply that ordering to the `_downstream`
        member field in the hope to speed up sorting the same list in the future using the `_sort_receivers` method.
        :param receivers: List of Receivers to emit to.
        :param value: Value to emit.
        """
        signal = ValueSignal(self.get_id(), value, is_blockable=False)
        for receiver in receivers:
            try:
                receiver.on_next(signal)
            except Exception as exception:
                self._error_handler(self.get_id(), receiver.get_id(), exception)

    def _connect(self, receiver: 'Receiver'):
        """
        Connects a new Receiver downstream.
        This method is only called from `Receiver.connect_to` and we can be sure that the Schemas match.
        :param receiver: New Receiver.
        """
        assert receiver.get_input_schema() != self.get_output_schema()

        if self.is_completed():
            receiver.on_complete(CompletionSignal(self.get_id()))

        else:
            weak_receiver = weak_ref(receiver)
            assert weak_receiver not in self._downstream
            self._downstream.append(weak_receiver)

    def _disconnect(self, receiver: 'Receiver'):
        """
        Removes the given Receiver if it is connected. If not, the call is ignored.
        :param receiver: Receiver to disconnect.
        """
        weak_receiver = weak_ref(receiver)
        if weak_receiver in self._downstream:
            self._downstream.remove(weak_receiver)

    def _collect_receivers(self) -> List['Receiver']:
        """
        Remove all expired Receivers and return a list of non-owning references to the valid ones (in Python, they are
        necessarily strong references, but in C++ we can make sure that the user cannot copy and/or store them in user-
        defined code).
        Note that the order of the upstream field does not change.
        """
        receivers = []
        valid_index = 0
        for current_index in range(len(self._downstream)):
            receiver = self._downstream[current_index]()
            if receiver is not None:
                receivers.append(receiver)
                self._downstream[valid_index] = self._downstream[current_index]  # would be a move or swap in C++
                valid_index += 1
        self._downstream = self._downstream[:valid_index]
        return receivers

    @staticmethod
    def _default_error_handler(emitter_id: int, receiver_id: int, error: Exception):
        """
        Default error handler. Does nothing but dump the exception traceback to stderr.
        All input arguments are ignored.
        :param emitter_id:  ID of the Emitter that caught the error.
        :param receiver_id: ID of the Receiver that produced the error.
        :param error:       The exception object.
        """
        print_error("Receiver {} failed during Logic evaluation.\nException caught by Emitter {}:\n{}".format(
            receiver_id, emitter_id, error))


########################################################################################################################

class Receiver(metaclass=ABCMeta):

    def __init__(self, schema: Value.Schema = Value.Schema()):
        """
        Constructor.
        :param schema:  Schema defining the Value expected by this Receiver, defaults to empty.
        """
        self._upstream: List[Emitter] = []  # Emitters are ordered but unique
        self._input_schema: Value.Schema = schema  # Is constant

    def get_input_schema(self) -> Value.Schema:
        """
        Schema of expected values. Can be the empty Schema.
        """
        return self._input_schema

    def get_id(self) -> int:
        """
        Unique identifier of this Circuit element.
        """
        return id(self)

    def on_next(self, signal: ValueSignal):
        """
        Called when an upstream Emitter has produced a new Value.
        :param signal   The Signal associated with this call.
        """
        # assert that the emitter is connected to this receiver
        for emitter in self._upstream:
            if emitter.get_id() == signal.get_source():
                break
        else:
            assert False

        # pass the signal along to the user-defined handler
        self._on_next(signal)

    def on_error(self, signal: ErrorSignal):
        """
        Called when an upstream Emitter has failed.
        :param signal       The ErrorSignal associated with this call.
        """
        # disconnect from the completed emitter
        self._disconnect_from(signal.get_source())

        # pass the signal along to the user-defined handler
        self._on_error(signal)

    def on_complete(self, signal: CompletionSignal):
        """
        Called when an upstream Emitter has finished successfully.
        :param signal       The CompletionSignal associated with this call.
        """
        # disconnect from the completed emitter
        self._disconnect_from(signal.get_source())

        # pass the signal along to the user-defined handler
        self._on_complete(signal)

    def connect_to(self, emitter: Emitter):
        """
        Connect to the given Emitter.
        :param emitter:     Emitter to connect to. If this Receiver is already connected, this does nothing.
        :raise TypeError:   If the Emitters's input Schema does not match.
        """
        # multiple connections between the same Emitter-Receiver pair are not allowed
        if emitter in self._upstream:
            return

        # check here that the schemas match, the emitter will assert that this is the case
        if self.get_input_schema() != emitter.get_output_schema():
            raise TypeError("Cannot connect an Emitter to a Receiver with a different Value Schema")

        # if the emitter is already completed, it will call `on_complete` right away, so we need to store the emitter
        # before connecting to it
        self._upstream.append(emitter)

        # connect to the emitter
        emitter._connect(self)

    def disconnect_from(self, emitter: Emitter):
        """
        Disconnects from the given Emitter.
        :param emitter:     Emitter to disconnect from. If this Receiver is not connected, this does nothing.
        """
        # do nothing if the emitter is not connected in the first place
        if emitter not in self._upstream:
            return

        # disconnect first, because removing the emitter may cause it to be deleted
        emitter._disconnect(self)

        # remove the emitter, potentially deleting it
        self._upstream.remove(emitter)

    # private
    @abstractmethod
    def _on_next(self, signal: ValueSignal):
        """
        Abstract method called by an upstream Emitter whenever a new Signal was produced.
        Would be pure virtual in C++.
        :param signal   The Signal associated with this call.
        """
        raise NotImplementedError()

    def _on_error(self, signal: ErrorSignal):
        """
        Default implementation of the "error" method called when an upstream Emitter has failed.
        By the time that this function is called, the Emitter is already disconnected.
        :param signal       The ErrorSignal associated with this call.
        """
        pass

    def _on_complete(self, signal: CompletionSignal):
        """
        Default implementation of the "complete" method called when an upstream Emitter has finished successfully.
        By the time that this function is called, the Emitter is already disconnected.
        :param signal       The CompletionSignal associated with this call.
        """
        pass

    def _disconnect_from(self, emitter_id: int):
        """
        Both `on_error` and `on_complete` automatically disconnect from the Emitter that produced the Signal.
        :param emitter_id: ID of the Emitter to disconnect from.
        """
        # find the completed emitter by the id contained in the signal
        for candidate_emitter in self._upstream:
            if candidate_emitter.get_id() == emitter_id:
                emitter = candidate_emitter
                break
        else:
            # the emitter must be connected to this receiver
            assert False

        # disconnect from the completed emitter
        self.disconnect_from(emitter)


########################################################################################################################

class Switch(Receiver, Emitter):
    class Operation(metaclass=ABCMeta):
        """
        Operations are Functors that can be part of a Switch.
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

        def __call__(self, value: Value) -> Optional[Value]:
            """
            Perform the Operation on a given value.
            :param value: Input value, must conform to the Schema specified by the `input_schema` property.
            :return: Either a new output value (conforming to the Schema specified by the `output_schema` property)
                or None.
            :raise TypeError: If the input Value does not conform to this Operation's input Schema.
            """
            if not isinstance(value, Value) or value.schema != self.get_input_schema():
                raise TypeError("Value does not conform to the Operation's input Schema")
            result: Optional[Value] = self._perform(value)
            if result is not None:
                assert result.schema == self.get_output_schema()
            return result
            pass

        # private
        @abstractmethod
        def _perform(self, value: Value) -> Optional[Value]:
            """
            Operation implementation.
            :param value: Input value, conforms to the input Schema.
            :return: Either a new output value conforming to the output Schema or None.
            """
            pass

    def __init__(self, *operations: Operation):
        """
        Constructor.
        :param operations: All Operations that this Switch performs in order. Must not be empty.
        :raise ValueError: If no Operations are passed.
        :raise TypeError: If two subsequent Operations have mismatched Schemas.
        """
        if len(operations) == 0:
            raise ValueError("Cannot create a Switch without a single Operation")

        for i in range(len(operations) - 1):
            if operations[i].output_schema != operations[i + 1].input_schema:
                raise TypeError(f"Operations {i} and {i + 1} have mismatched Value Schemas")

        self._operations: Tuple[Switch.Operation] = operations

        Receiver.__init__(self, self._operations[0].get_input_schema())
        Emitter.__init__(self, self._operations[-1].get_output_schema())

    def _operate_on(self, value: Value) -> Optional[Value]:
        """
        Performs the sequence of Operations on the given Value and returns it or None.
        Exceptions thrown by a Operation will propagate back to the caller.
        :param value: Input value.
        :return: Output of the last Operation or None.
        """
        for operation in self._operations:
            value = operation(value)
            if value is None:
                break
        return value

    def _on_next(self, signal: ValueSignal):
        """
        Performs the sequence of Operations on the given Value and emits the result if one is produced.
        Exceptions thrown by a Operation will cause the Switch to fail.

        :param signal   The Signal associated with this call.
        """
        try:
            result = self._operate_on(signal.get_value())
        except Exception as exception:
            self._error(exception)
        else:
            if result is not None:
                self.emit(result)
