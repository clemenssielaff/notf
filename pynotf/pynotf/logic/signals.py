from enum import Enum

from ..value import Value

from .circuit import Circuit


########################################################################################################################

class CompletionSignal:
    """
    Signal emitted by an Emitter that produced the last value in its data stream.
    This is the last Signal to be emitted by that Emitter.
    """

    def __init__(self, emitter_id: Circuit.Element.ID):
        """
        Constructor.
        :param emitter_id:      ID of the Emitter that emitted the Signal.
        """
        self._emitter_id: Circuit.Element.ID = emitter_id  # is constant

    def get_source(self) -> Circuit.Element.ID:
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

    def __init__(self, emitter_id: Circuit.Element.ID, error: Exception):
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
    Must be copyable, so that Node Callbacks can work on a copy and the change is only applied to the original Signal if
    the Callback succeeded. It doesn't need to be assignable though.
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

    def __init__(self, emitter_id: Circuit.Element.ID, value: Value, is_blockable: bool = False):
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
