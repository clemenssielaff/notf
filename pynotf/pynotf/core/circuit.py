from __future__ import annotations

from collections import deque
from weakref import ref as weak_ref
from logging import warning
from enum import Enum, IntEnum, unique, auto
from threading import Condition
from typing import List, Optional, NamedTuple, Union, Deque, Any

from pyrsistent import field

from pynotf.data import Value, RowHandle, RowHandleList, Table, TableRow, Storage
import pynotf.core as core


# DATA #################################################################################################################

# All (public) columns of the Emitter table.
class EmitterColumns(TableRow):
    __table_index__: int = core.TableIndex.EMITTERS  # in C++ this would be a type trait
    schema = field(type=Value.Schema, mandatory=True)  # The Schema of Values emitted downstream.
    value = field(type=Value, mandatory=True)  # The last emitted Value, undefined until first emission.
    downstream = field(type=RowHandleList, mandatory=True, initial=RowHandleList())  # Set of downstream Receivers.
    flags = field(type=int, mandatory=True, initial=0)  # bitset:
    # | 4 | 3 | 2 | 1 | 0 |
    #   +---+---+   |   |
    #       |       |  is blockable?
    #       |       has emitted?
    #       |
    #      status


# The first two (public) columns in any Receiver table.
class ReceiverRow(TableRow):
    schema = field(type=Value.Schema, mandatory=True)  # The Schema of Values received from upstream.


class FlagIndex(IntEnum):
    """
    IS_BLOCKABLE
    True iff the emitted Signals can be blocked by a Receiver downstream.

    HAS_EMITTED
    If an Emitter has emitted once and a new Receiver connects to it, it will receive the latest Value right away - but
    only if the Emitter has emitted at least once before.

    STATUS
    See `EmitterStatus` below.
    """
    IS_BLOCKABLE = 0
    HAS_EMITTED = 1
    STATUS = 2
    _LAST = 2


@unique
class EmitterStatus(IntEnum):
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
    FAILED = 2
    FAILING = 3
    COMPLETED = 4
    COMPLETING = 5

    def is_active(self) -> bool:
        """
        There are 3 active and 3 passive states:
            * IDLE, FAILED and COMPLETED are passive
            * EMITTING, FAILING and COMPLETING are active
        """
        return self in (EmitterStatus.EMITTING,
                        EmitterStatus.FAILING,
                        EmitterStatus.COMPLETING)

    def is_completed(self) -> bool:
        """
        Every status except IDLE and EMITTING can be considered "completed".
        """
        return not (self == EmitterStatus.IDLE or self == EmitterStatus.EMITTING)


class TopologyChange(NamedTuple):
    """
    Topology changes in the Circuit are delayed until the end of an Event to ensure that the Logic behaves consistently.
    """

    @unique
    class Kind(Enum):
        """
        There are only two kinds of changes to a DAG: the creation of a connection or the removal of an existing one.
        """
        CREATE_CONNECTION = auto()
        REMOVE_CONNECTION = auto()

    emitter: RowHandle  # Row of the Emitter at the source of the connection.
    receiver: RowHandle  # Row of the Receiver at the target of the connection.
    kind: Kind


class Event(NamedTuple):
    """
    Queue-able objects that introduce a new Value-/Failure-/Completion-Signal into the Circuit.
    """
    emitter: RowHandle  # Row of the Emitter at the source of the connection.
    value: Union[Value, Exception, None] = None


class CompletionSignal(NamedTuple):
    emitter: RowHandle


class FailureSignal(NamedTuple):
    emitter: RowHandle
    error: Exception


class ValueSignal(NamedTuple):
    emitter: RowHandle
    value: Value
    status: Status

    @unique
    class Status(Enum):
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


class CircuitData(NamedTuple):
    table: Table
    events: Deque[Event] = deque()
    event_condition: Condition = Condition()
    topology_changes: List[TopologyChange] = []  # changes to the topology during an event in order


# FUNCTIONS ############################################################################################################


def get_flag(table: Table, handle: RowHandle, flag_index: int) -> bool:
    assert 0 <= flag_index <= FlagIndex._LAST
    return bool((table[handle]['flags'] >> flag_index) & 1)


# def set_flag(table: Table, handle: RowHandle, flag_index: int, flag: bool) -> None:
#     assert 0 <= flag_index <= FlagIndex._LAST
#     current_flags: int = table[handle]['flags']
#     table[handle]['flags'] = current_flags ^ ((-int(flag) ^ current_flags) & (1 << flag_index))
#
def get_schema(table: Table, handle: RowHandle) -> Value.Schema:
    return table[handle]['schema']


#
# def set_schema(table: Table, handle: RowHandle, schema: Value.Schema) -> None:
#     table[handle]['schema'] = schema
#
#
def has_downstream(table: Table, emitter: RowHandle) -> bool:
    return len(table[emitter]['downstream']) > 0


def get_downstream(table: Table, emitter: RowHandle) -> RowHandleList:
    return table[emitter]['downstream']


def is_downstream(table: Table, emitter: RowHandle, receiver: RowHandle) -> bool:
    return receiver in table[emitter]["downstream"]


def connect(table: Table, emitter: RowHandle, receiver: RowHandle) -> None:
    if not is_downstream(table, emitter, receiver):
        table[emitter]["downstream"] = table[emitter]["downstream"].append(receiver)


def clear_downstream(table: Table, emitter: RowHandle) -> None:
    table[emitter]["downstream"] = RowHandleList()


#
# def get_value(table: Table, handle: RowHandle) -> Optional[Value]:
#     return table[handle]['value'] if get_flag(table, handle, FlagIndex.HAS_EMITTED) else None
#
#
# def set_value(table: Table, handle: RowHandle, value: Value) -> None:
#     table[handle]['value'] = value
#     set_flag(table, handle, FlagIndex.HAS_EMITTED, True)
#
def is_blockable(table: Table, handle: RowHandle) -> bool:
    return get_flag(table, handle, FlagIndex.IS_BLOCKABLE)


#
#
# def set_constants(table: Table, handle: RowHandle, *_, emitter: bool = False, receiver: bool = False,
#                   blockable: bool = False, external: bool = False) -> None:
#     constants: int = (int(emitter) << FlagIndex.IS_EMITTER |
#                       int(receiver) << FlagIndex.IS_RECEIVER |
#                       int(blockable) << FlagIndex.IS_BLOCKABLE |
#                       int(external) << FlagIndex.IS_EXTERNAL)
#     table[handle]['flags'] = (table[handle]['flags'] & ~int(0b1111)) | constants


def get_status(table: Table, handle: RowHandle) -> EmitterStatus:
    return EmitterStatus(table[handle]['flags'] >> FlagIndex.STATUS)


def set_status(table: Table, handle: RowHandle, status: EmitterStatus) -> None:
    current_flags: int = table[handle]['flags']
    table[handle]['flags'] = (current_flags & ~(int(0b111) << FlagIndex.STATUS)) | (status << FlagIndex.STATUS)


def emit_value(table: Table, emitter: RowHandle, value: Value) -> Optional[core.Error]:
    """
    Push the given value to all active Receivers.
    :param table:   The Emitter table.
    :param emitter: Handle to the row of the Emitter.
    :param value:   Value to emit, can be empty if the Emitter does not emit a meaningful value.
    :return:        Error if something goes wrong, otherwise None.
    """
    assert get_schema(table, emitter) == value.get_schema()

    # make sure we can never emit a Value once the Emitter has completed
    status: EmitterStatus = get_status(table, emitter)
    if status.is_completed():
        return

    # early out if there wouldn't be a point of emitting anyway
    if not has_downstream(table, emitter):
        return

    # make sure that we are not already emitting
    if status != EmitterStatus.IDLE:
        return core.Error(emitter, core.Error.Kind.NO_DAG,
                          f"Cyclic dependency detected during emission from Emitter {emitter.index}.")
    set_status(table, emitter, EmitterStatus.EMITTING)

    # create the signal to emit, in C++ the value would be moved
    signal = ValueSignal(
        emitter, value,
        ValueSignal.Status.UNHANDLED if is_blockable(table, emitter) else ValueSignal.Status.UNBLOCKABLE)

    # emit to all receivers in order
    for receiver in get_downstream(table, emitter):
        # highly unlikely, but can happen if there is a cycle in the circuit that caused this emitter to
        # complete while it is in the process of emitting a value
        if get_status(table, emitter).is_completed():
            return

        # error = receiver.on_value(signal)  TODO: Receiver callbacks (& exception safety ?)

        # stop the emission if the Signal was blocked
        # C++ will most likely be able to move this condition out of the loop
        if is_blockable(table, emitter) and signal.status == ValueSignal.Status.BLOCKED:
            break

    # reset the emission flag if we are still emitting
    if get_status(table, emitter) == EmitterStatus.EMITTING:
        set_status(table, emitter, EmitterStatus.IDLE)


def emit_failure(table: Table, emitter: RowHandle, error: Exception) -> Optional[core.Error]:
    """
    Failure method, completes the Emitter while letting the downstream Receivers inspect the error.
    :param table:   The Emitter table.
    :param emitter: Handle to the row of the Emitter.
    :param error:   The error that has occurred.
    :return:        Error if something goes wrong, otherwise None.
    """
    # make sure we can never emit failure again, once the Emitter has completed
    if get_status(table, emitter).is_completed():
        return

    # we don't have to test for cyclic dependency errors here because this method will complete the emitter
    set_status(table, emitter, EmitterStatus.FAILING)

    # create the signal
    signal: FailureSignal = FailureSignal(emitter, error)

    # emit to all receivers in order
    for receiver in get_downstream(table, emitter):
        # error = receiver.on_failure(signal)   TODO: Receiver callbacks (& exception safety ?)
        pass

    # simply complete the emitter, we don't care about the data remaining in the row
    set_status(table, emitter, EmitterStatus.FAILED)

    return None


def emit_completion(table: Table, emitter: RowHandle) -> Optional[core.Error]:
    """
    Failure method, completes the Emitter while letting the downstream Receivers inspect the error.
    :param table:   The Emitter table.
    :param emitter: Handle to the row of the Emitter.
    :return:        Error if something goes wrong, otherwise None.
    """
    # It is possible to emit completion multiple times, if the Emitter has just completed but is still around so
    # Receivers can still connect to it.

    # we don't have to test for cyclic dependency errors here because this method will complete the emitter
    set_status(table, emitter, EmitterStatus.COMPLETING)

    # create the signal
    signal: CompletionSignal = CompletionSignal(emitter)

    # emit to all receivers in order
    for receiver in get_downstream(table, emitter):
        # error = receiver.on_completion(signal)   TODO: Receiver callbacks (& exception safety ?)
        pass

    # (re-) complete the emitter
    set_status(table, emitter, EmitterStatus.COMPLETED)

    return None


# API ##################################################################################################################

class Circuit:
    """
    Circuit object accessible only by the Application.
    Owns the only strong reference to a `CircuitData` instance.
    """

    EmitterColumns = EmitterColumns

    def __init__(self, application: core.Application):
        self._application: core.Application = application
        self._storage: Storage = application.get_storage()
        self._data: CircuitData = CircuitData(self._storage[core.TableIndex.EMITTERS])

    def create_fact(self, schema: Value.Schema) -> Fact:
        return Fact(self._data, schema)

    def await_event(self, timeout: Optional[float] = None) -> Optional[Event]:
        """
        Blocks the calling thread until a time where at least one new event has been queued in the Circuit.
        :param timeout: Optional timeout in seconds.
        """
        with self._data.event_condition:
            if not self._data.event_condition.wait_for(lambda: len(self._data.events) > 0, timeout):
                return None
            return self._data.events.popleft()

    def handle_event(self, event: Event) -> None:
        """
        Handles the next Event.
        Event handling and topology changes are not thread-safe and should only be called from a single event loop.
        :param event: Event to handle.
        """
        # at this point, we should have no outstanding topology changes to perform
        assert len(self._data.topology_changes) == 0

        # do nothing, if the emitter is invalid
        if not self._data.table.is_handle_valid(event.emitter):
            return

        # store the application state
        restoration_point = self._storage.get_data()

        # handle the event
        error: Optional[core.Error] = None
        if isinstance(event.value, Value):
            error = emit_value(self._data.table, event.emitter, event.value)
        elif isinstance(event.value, Exception):
            error = emit_failure(self._data.table, event.emitter, event.value)
        else:
            assert event.value is None
            error = emit_completion(self._data.table, event.emitter)

        # reset the application state on error
        if error is not None:
            self._storage.set_data(restoration_point)
            return self._application.handle_error(error)

        # perform delayed topology changes, this might cause new Events to be created
        self.apply_topology_changes()

    def apply_topology_changes(self) -> None:
        """
        Perform all delayed changes to the topology of the Circuit.
        There should be no need to call this manually outside of testing.
        Event handling and topology changes are not thread-safe and should only be called from a single event loop.
        """
        for change in self._data.topology_changes:

            # the affected topology might not even exist anymore
            if not (self._data.table.is_handle_valid(change.emitter) and
                    self._data.table.is_handle_valid(change.receiver)):
                continue
            assert self._data.refcounts[change.emitter.index] > 0
            assert self._data.refcounts[change.receiver.index] > 0

            # perform the topology change
            if change.kind == TopologyChange.Kind.CREATE_CONNECTION:
                self._create_connection(change.emitter, change.receiver)
            else:
                assert change.kind == TopologyChange.Kind.REMOVE_CONNECTION
                self._remove_connection(change.emitter, change.receiver)

        self._data.topology_changes.clear()

    # private

    def _create_connection(self, emitter: RowHandle, receiver: RowHandle) -> None:
        """
        Creates a new connection in the Circuit.
        :param emitter: Emitter at the source of the connection.
        :param receiver: Receiver at the target of the connection.
        """
        # Never create a connection mid-event.
        emitter_status: EmitterStatus = get_status(self._data.table, emitter)
        assert not emitter_status.is_active()

        # Multiple connections between the same Emitter-Receiver pair are ignored.
        if is_downstream(self._data.table, emitter, receiver):
            return  # This is only possible iff the Emitter has not yet completed.

        # Add the Receiver as a downstream connection to the Emitter, even if it has already completed.
        connect_downstream(self._data.table, emitter, receiver)

        if emitter_status.is_completed():
            # A completed Emitter must be external since all connections into the Circuit should have been removed.
            assert is_external(self._data.table, emitter)
            assert self._data.refcounts[emitter.index] == 1

            # If the Emitter has already completed, schedule a CompletionSignal as a follow-up of this Event.
            with self._data.event_condition:
                # TODO: ensure that a completed Emitter emits a "non-spontaneous?" signal
                self._data.events.append(Event(emitter))
                self._data.event_condition.notify_all()

        # If the Emitter is still alive, add an upstream connection from the Receiver to it.
        else:
            # It is possible that the Receiver already has a strong reference to the Emitter if it just created it.
            connect_upstream(self._data.table, receiver, emitter)
            self._data.refcounts[emitter.index] += 1

    def _remove_connection(self, emitter: RowHandle, receiver: RowHandle):
        """
        Removes a connection in the Circuit.
        :param emitter: Emitter at the source of the connection.
        :param receiver: Receiver at the target of the connection.
        """
        # TODO: CONTINUE HERE
        if emitter in receiver._upstream:
            receiver._upstream.remove(emitter)

            # mark the emitter as a candidate for deletion
            self.expire_emitter(emitter)

        weak_receiver: weak_ref = weak_ref(receiver)
        if weak_receiver in emitter._downstream:
            emitter._downstream.remove(weak_ref(receiver))


class Fact:
    """
    Facts represent changeable, external truths that the Application Logic can react to.
    To the Application Logic they appear as simple Emitters that are owned and managed by a single Service in a
    thread-safe fashion. The Service will update the Fact as new information becomes available, completes the Fact's
    Emitter when the Service is stopped or the Fact has expired (for example, when a sensor is disconnected) and
    forwards appropriate exceptions to the Receivers of the Fact should an error occur.
    Examples of Facts are:
        - the latest reading of a sensor
        - the user performed a mouse click (incl. position, button and modifiers)
        - the complete chat log
        - the expiration signal of a timer
    Facts consist of a Value, possibly the empty Value. Empty Facts simply informing the logic that an event has
    occurred without additional information.
    """

    def __init__(self, circuit: CircuitData, schema: Value.Schema):
        self._circuit: weak_ref = weak_ref(circuit)
        self._schema: Value.Schema = schema
        self._emitter: RowHandle = RowHandle()  # invalid by default

        with circuit.event_condition:  # TODO: it would be nice if this wasn't blocking (we need async for that)
            pass  # TODO: create element

    def __del__(self):
        self.remove()

    def emit_value(self, value: Any = Value()) -> None:
        """
        Schedules the emission of a Value into the Circuit.
        Does nothing if the Fact is invalid.
        :param value:       Value to emit. Must match this Fact's Schema.
        :raise TypeError:   If the Value's Schema does not match this Fact.
        """
        circuit: Optional[CircuitData] = self._check_validity()
        if circuit is None:
            return

        # implicit value conversion
        if not isinstance(value, Value):
            try:
                value = Value(value)
            except ValueError:
                raise TypeError("Emitters can only emit Values or things that are implicitly convertible to one")
            # TODO: central error handling by the application instead of exceptions?

        # ensure that the value can be emitted by the Emitter
        emitter_schema: Value.Schema = get_schema(circuit.table, self._emitter)
        if emitter_schema != value.get_schema():
            raise TypeError(f"Cannot emit Value from Emitter {self._emitter.index}."
                            f"  Emitter schema: {emitter_schema}"
                            f"  Value schema: {value.get_schema()}")

        # schedule the event
        with circuit.event_condition:
            circuit.events.append(Event(self._emitter, value))
            circuit.event_condition.notify_all()

    def emit_failure(self, emitter: RowHandle, error: Exception):
        """
        Schedules the failure of an Emitter in the Circuit.
        :param emitter: Emitter to fail.
        :param error:   Error causing the failure.
        """
        circuit: Optional[CircuitData] = self._check_validity()
        if circuit is None:
            return

        with circuit.event_condition:
            circuit.events.append(Event(emitter, error))  # TODO: "Exception" object instead of Python Exception?
            circuit.event_condition.notify_all()

    def emit_completion(self, emitter: RowHandle):
        """
        Schedules the voluntary completion of an Emitter in the Circuit.
        :param emitter: Emitter to complete.
        """
        circuit: Optional[CircuitData] = self._check_validity()
        if circuit is None:
            return

        with circuit.event_condition:
            circuit.events.append(Event(emitter))
            circuit.event_condition.notify_all()

    def remove(self) -> None:
        circuit: Optional[CircuitData] = self._circuit()
        if circuit:
            with circuit.event_condition:  # TODO: it would be nice if this wasn't blocking (we need async for that)
                pass  # TODO: remove element
            self._emitter = RowHandle()  # TODO: do we need to worry about thread safety during Fact removal?
        assert not self._emitter

    def _get_circuit(self) -> Optional[CircuitData]:
        if not self._emitter:
            return None

        circuit: Optional[CircuitData] = self._circuit()
        if circuit:
            return circuit
        else:
            self._emitter = RowHandle()
            return None

    def _check_validity(self) -> Optional[CircuitData]:
        circuit: Optional[CircuitData] = self._circuit()
        if circuit is None:
            warning(f'Cannot emit from a Fact of an invalid Circuit')
            return None

        # it is possible that an Emission is scheduled for an Emitter that has already been removed
        handle_error: Optional[str] = circuit.table.check_handle(self._emitter)
        if handle_error:
            warning(f'Cannot emit from invalid Emitter: {handle_error}')
            return None

        return circuit

# class CircuitOld:
#
#     # public for elements
#     def create_edge(self, emitter: RowHandle, receiver: RowHandle):
#         """
#         Schedules the addition of an edge in the Circuit.
#         :param emitter:     Emitter at the source of the edge.
#         :param receiver:    Receiver at the target of the edge.
#         """
#         self._topology_changes.append(TopologyChange(emitter, receiver, TopologyChange.Kind.CREATE_CONNECTION))
#
#     def remove_edge(self, emitter: RowHandle, receiver: RowHandle):
#         """
#         Schedules the removal of an edge in the Circuit.
#         :param emitter:     Emitter at the source of the edge.
#         :param receiver:    Receiver at the target of the edge.
#         """
#         self._topology_changes.append(TopologyChange(emitter, receiver, TopologyChange.Kind.REMOVE_CONNECTION))
#
#     def expire_emitter(self, emitter: 'Emitter'):
#         """
#         Passes ownership of a (potentially) expired Emitter to the Circuit so it can be deleted when the application is
#         idle. If there are other owners of the Emitter, this does nothing.
#         :param emitter: Emitter to expire (in C++ this would be an r-value).
#         """
#         self._expired_elements.append(emitter)
