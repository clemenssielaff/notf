from enum import Enum
from typing import Optional, List
from weakref import ref as weak_ref

from pynotf.data import Value

from .circuit import Circuit
from .signals import ValueSignal, FailureSignal, CompletionSignal


########################################################################################################################
# noinspection PyAbstractClass
class Emitter(Circuit.Element):
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

        def __init__(self, emitter: 'Emitter'):
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

        def get_id(self) -> Optional[Circuit.Element.ID]:
            """
            Returns the Element ID of the Emitter or None if the handle has expired.
            """
            emitter: Emitter = self._element()
            if emitter is None:
                return None
            else:
                return emitter.get_id()

        def get_output_schema(self) -> Optional[Value.Schema]:
            """
            Returns the output schema of the Emitter or None, if the handle has expired.
            """
            emitter: Emitter = self._element()
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
        self._status: Emitter._Status = self._Status.IDLE

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
    def _emit(self, value: Value = Value()):  # noexcept
        """
        Push the given value to all active Receivers.

        :param value:       Value to emit, can be empty if this Emitter does not emit a meaningful value.
        """
        assert value.get_schema() == self.get_output_schema()

        # make sure we can never emit once the Emitter has completed
        if self.is_completed():
            return

        # early out if there wouldn't be a point of emitting anyway
        if not self.has_downstream():
            return

        # make sure that we are not already emitting
        if self._status != self._Status.IDLE:
            return self.get_circuit().handle_error(Circuit.Error(
                weak_ref(self),
                Circuit.Error.Kind.NO_DAG,
                f"Cyclic dependency detected during emission from Emitter {self.get_id()}."))
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
                receiver: Optional[Receiver] = weak_receiver()
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
                receiver: Optional[Receiver] = weak_receiver()
                if receiver is None:
                    continue

                receiver.on_completion(signal)

        finally:
            # complete the emitter
            self._downstream.clear()
            self._status = self._Status.COMPLETED


########################################################################################################################

from .receiver import Receiver
