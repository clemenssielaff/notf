from abc import ABCMeta, abstractmethod
from enum import Enum
from inspect import signature
from logging import error as print_error
from typing import Any, Optional, List, Callable, Tuple, Union, Iterable, Set
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
        self._emitter_id: int = emitter_id  # is constant
        self._value: Value = value  # is constant
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

class FailureSignal:
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
        self._emitter_id: int = emitter_id  # is constant
        self._error: Exception = error  # is constant

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

class Emitter:
    """
    Circuit element pushing a Signal downstream.
    Is non-copyable and may only be owned by downstream Receivers and Nodes.
    """

    class _OrderableReceivers:
        def __init__(self, receivers: List['Receiver']):
            """
            Constructor.
            :param receivers: Receivers to sort.
            """
            self._receivers: List[Receiver] = receivers

        def __len__(self) -> int:
            """
            :return: Number of elements in the list.
            """
            return len(self._receivers)

        def __getitem__(self, index: Union[int, slice]) -> 'Receiver':
            """
            [] access operator.
            :param index: Index (-slice) to return.
            :raise IndexError: If the index is illegal.
            :return: Element/s at the requested index/indices.
            """
            return self._receivers[index]

        def __iter__(self) -> Iterable['Receiver']:
            """
            :return: Iterator through the list.
            """
            yield from self._receivers

        def __reversed__(self) -> Iterable['Receiver']:
            """
            :return Reverse-iterator through the list:
            """
            return reversed(self._receivers)

        def index(self, item: 'Receiver', start: Optional[int] = None, end: Optional[int] = None) -> int:
            """
            :param item: Item to look for.
            :param start: Start index to start limit the search space to the left.
            :param end: End index to limit the search space to the right.
            :raise ValueError: If no entry equal to item was found in the inspected subsequence.
            :return: Return zero-based index in the list of the first entry that is equal to item.
            """
            return self._receivers.index(item,
                                         start if start is not None else 0,
                                         end if end is not None else len(self._receivers))

        def move_to_front(self, item: 'Receiver'):
            """
            :param item: Moves this item to the front of the list.
            :raise ValueError: If no such item is in the list.
            """
            self._rotate(0, self.index(item) + 1, 1)

        def move_to_back(self, item: 'Receiver'):
            """
            :param item: Moves this item to the back of the list.
            :raise ValueError: If no such item is in the list.
            """
            self._rotate(self.index(item), len(self._receivers), -1)

        def move_in_front_of(self, item: 'Receiver', other: 'Receiver'):
            """
            :param item: Moves this item right in front of the other.
            :param other: Other item to move in front of.
            :raise ValueError: If either item is not in the list.
            """
            item_index: int = self.index(item)
            other_index: int = self.index(other)
            if item_index == other_index:
                return
            elif item_index > other_index:
                self._rotate(other_index, item_index + 1, 1)
            else:
                self._rotate(item_index, other_index, -1)

        def move_behind_of(self, item: 'Receiver', other: 'Receiver'):
            """
            :param item: Moves this item right behind the other.
            :param other: Other item to move behind.
            :raise ValueError: If either item is not in the list.
            """
            item_index: int = self.index(item)
            other_index: int = self.index(other)
            if item_index == other_index:
                return
            elif item_index > other_index:
                self._rotate(other_index + 1, item_index + 1, 1)
            else:
                self._rotate(item_index, other_index + 1, -1)

        def _rotate(self, start: int, end: int, steps: int):
            """
            Rotates the given sequence to the right.
            Example:
                my_list = [0, 1, 2, 3, 4, 5, 6]
                _rotate(my_list, 2, 5, 1)  # returns [0, 1, 4, 2, 3, 5, 6]
            :param start:       First index of the rotated subsequence.
            :param end:         One-past the last index of the rotated subsequence.
            :param steps:       Number of steps to rotate.
            :return:            The modified list.
            """
            pivot: int = (end if steps >= 0 else start) - steps
            self._receivers = (self._receivers[0:start] + self._receivers[pivot:end] +
                               self._receivers[start:pivot] + self._receivers[end:])

    def __init__(self, schema: Value.Schema = Value.Schema(), is_blockable: bool = False):
        """
        Constructor.
        :param schema:          Schema of the Value emitted by this Emitter, defaults to empty.
        :param is_blockable:    Whether or not Signals from this Emitter can be blocked.
        """
        self._downstream: List[weak_ref] = []  # subclasses may only change the order
        self._output_schema: Value.Schema = schema  # is constant
        self._is_blockable: bool = is_blockable  # is constant
        self._is_completed: bool = False  # once a Emitter is completed, it can no longer emit any values
        self._is_emitting: bool = False  # is set to true during emission to catch cyclic dependency errors
        self._error_handler: Callable = self._default_error_handler  # cannot be empty

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

    def is_completed(self) -> bool:
        """
        Returns true iff the Emitter has been completed, either through an error or normally.
        """
        return self._is_completed

    def has_downstream(self) -> bool:
        """
        Checks if emitting a Value through this Emitter reaches any Receivers downstream.
        This way you can skip expensive calculations of Values if there is nobody to receive them anyway.
        """
        return len(self._downstream) > 0

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

    # protected
    def _emit(self, value: Any = Value()):
        """
        Push the given value to all active Receivers.
        :param value: Value to emit, can be empty if this Emitter does not emit a meaningful value.
        :raise TypeError: If the Value's Schema doesn't match.
        :raise RuntimeError: If the Emitter has already completed (either normally or though an error).
        """
        assert not self.is_completed()

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

        # make sure that we are not already emitting
        if self._is_emitting:  # this would be the job of a RAII guard in C++
            raise RuntimeError("Cyclic dependency detected during emission from Emitter {}.".format(self.get_id()))
        self._is_emitting = True

        try:
            # sort out invalid receivers and compile a list of strong references to live ones
            receivers: List[Receiver] = []
            highest_valid_index: int = 0
            for weak_receiver in self._downstream:
                receiver: Optional[Receiver] = weak_receiver()
                if receiver is not None:
                    receivers.append(receiver)
                    self._downstream[highest_valid_index] = weak_receiver  # could be a move or swap in C++
                    highest_valid_index += 1
            self._downstream = self._downstream[:highest_valid_index]

            # pre-emission allowing user-code to prepare the Emitter (change the order of receivers, for example)
            self._sort_receivers(self._OrderableReceivers(receivers))

            # create the signal to emit
            signal = ValueSignal(self.get_id(), value, self._is_blockable)

            # emit to all receivers in order
            for receiver in receivers:
                try:
                    receiver.on_next(signal)
                # pass exceptions to the assigned error handle
                except Exception as exception:
                    self._error_handler(self.get_id(), receiver.get_id(), exception)

                # stop the emission if the Signal was blocked
                if signal.is_blocked():
                    break

        finally:
            # reset the emission flag
            self._is_emitting = False

    def _fail(self, error: Exception):
        """
        Failure method, completes the Emitter while letting the downstream Receivers inspect the error.
        :param error:   The error that has occurred.
        """
        assert not self.is_completed()

        # make sure that we are not already emitting
        if self._is_emitting:
            raise RuntimeError("Cyclic dependency detected during emission from Emitter {}.".format(self.get_id()))
        self._is_emitting = True

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
                    self._error_handler(self.get_id(), receiver.get_id(), exception)

        finally:
            # complete the emitter
            self._downstream.clear()
            self._is_completed = True

            # reset the emission flag
            self._is_emitting = False

    def _complete(self):
        """
        Completes the Emitter successfully.
        """
        assert not self.is_completed()

        # make sure that we are not already emitting
        if self._is_emitting:
            raise RuntimeError("Cyclic dependency detected during emission from Emitter {}.".format(self.get_id()))
        self._is_emitting = True

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
                    self._error_handler(self.get_id(), receiver.get_id(), exception)

        finally:
            # complete the emitter
            self._downstream.clear()
            self._is_completed = True

            # reset the emission flag
            self._is_emitting = False

    # private
    def _sort_receivers(self, receivers: _OrderableReceivers):
        """
        Virtual method to allow user-defined code to re-order the Receivers prior to the emission of a value.
        :param receivers: Receivers of this Emitter, can only be re-ordered.
        """
        pass

    def _connect(self, receiver: 'Receiver'):
        """
        Connects a new Receiver downstream.
        This method is only called from `Receiver.connect_to` and we can be sure that the Schemas match.
        :param receiver: New Receiver.
        """
        assert receiver.get_input_schema() == self.get_output_schema()
        assert not self._is_emitting

        if self.is_completed():
            receiver.on_completion(CompletionSignal(self.get_id()))

        else:
            weak_receiver = weak_ref(receiver)
            assert weak_receiver not in self._downstream
            self._downstream.append(weak_receiver)

    def _disconnect(self, receiver: 'Receiver'):
        """
        Removes the given Receiver if it is connected. If not, the call is ignored.
        :param receiver: Receiver to disconnect.
        """
        assert not self._is_emitting
        self._downstream.remove(weak_ref(receiver))

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

    def __init__(self, circuit: 'Circuit', schema: Value.Schema = Value.Schema()):
        """
        Constructor.
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

    def on_failure(self, signal: FailureSignal):
        """
        Called when an upstream Emitter has failed.
        :param signal       The ErrorSignal associated with this call.
        """
        # disconnect from the completed emitter
        self.disconnect_from(signal.get_source())

        # pass the signal along to the user-defined handler
        self._on_failure(signal)

    def on_completion(self, signal: CompletionSignal):
        """
        Called when an upstream Emitter has finished successfully.
        :param signal       The CompletionSignal associated with this call.
        """
        # disconnect from the completed emitter
        self.disconnect_from(signal.get_source())

        # pass the signal along to the user-defined handler
        self._on_completion(signal)

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

        # if the emitter is already completed, it will call `on_completion` right away, so we need to store the emitter
        # before connecting to it
        self._upstream.add(emitter)
        # if emitter.is_completed():
        #     self._on_completion(CompletionSignal(emitter.get_id()))


        # connect to the emitter
        emitter._connect(self)

    def disconnect_from(self, emitter: Union[int, Emitter]):
        """
        Disconnects from the given Emitter.
        :param emitter: The emitter (or its id) to disconnect from.
        """
        # find the emitter to disconnect from
        if isinstance(emitter, int):
            for candidate in self._upstream:
                if candidate.get_id() == emitter:
                    emitter: Emitter = candidate
                    break
        assert emitter in self._upstream

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

    def _on_failure(self, signal: FailureSignal):
        """
        Default implementation of the "error" method called when an upstream Emitter has failed.
        By the time that this function is called, the Emitter is already disconnected.
        :param signal       The ErrorSignal associated with this call.
        """
        pass

    def _on_completion(self, signal: CompletionSignal):
        """
        Default implementation of the "complete" method called when an upstream Emitter has finished successfully.
        By the time that this function is called, the Emitter is already disconnected.
        :param signal       The CompletionSignal associated with this call.
        """
        pass


########################################################################################################################

class Circuit:
    def __init__(self):
        self._connections_to_remove: Set[Tuple[Emitter, Receiver]] = set()
        self._connections_to_create: Set[Tuple[Emitter, Receiver]] = set()

    def create_connection(self, emitter: Emitter, receiver: Receiver):
        self._connections_to_create.add((emitter, receiver))

    def remove_connection(self, emitter: Emitter, receiver: Receiver):
        self._connections_to_remove.add((emitter, receiver))

    def _create_connection(self, emitter: Emitter, receiver: Receiver):
        # multiple connections between the same Emitter-Receiver pair are not allowed
        if emitter in receiver._upstream:
            return


    def cleanup(self):
        # first add new connections because removing some might destroy Emitters



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
            self._fail(exception)
        else:
            if result is not None:
                self._emit(result)
