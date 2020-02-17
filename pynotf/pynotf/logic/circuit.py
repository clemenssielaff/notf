from abc import ABC, abstractmethod
from collections import deque
from enum import Enum
from logging import error as print_error
from threading import Lock, Condition
from typing import Any, Optional, List, Tuple, Deque, Dict, Callable, Type, TypeVar
from typing import NamedTuple, NewType
from weakref import ref as weak_ref

from pynotf.data import Value

T = TypeVar('T')

########################################################################################################################

ID = NewType('ID', int)
"""
The Emitter ID is a 64bit wide, ever-increasing unsigned integer that is unique in the Circuit.
"""


########################################################################################################################

class Element(ABC):
    """
    Base class for all Circuit Elements.
    This is an empty abstract base class that is inherited by the Emitter and Receiver interfaces.
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

class Error(NamedTuple):
    """
    Object wrapping any exception thrown (and caught) in the Circuit alongside additional information.
    """

    class Kind(Enum):
        NO_DAG = 1  # a cycle was detected during Event handling
        WRONG_VALUE_SCHEMA = 2  # the Schema of a Value did not match the expected
        USER_CODE_EXCEPTION = 3  # an exception was propagated out of user-defined Code

    element: weak_ref  # weak reference to the element
    kind: Kind  # kind of error caught in the circuit
    message: str  # error message


########################################################################################################################

class Circuit:
    """
    The Circuit object is a central object shared by all Circuit elements conceptually, even though only Receivers have
    a need for a reference to the Circuit object.
    Its main objective is to provide the handling of individual events and act as a central repository for named
    Operators.
    """

    Error = Error
    Element = Element

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
        def _get_emitter(self) -> Optional['Emitter']:
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

        def get_endpoints(self) -> Optional[Tuple['Emitter', 'Receiver']]:
            """
            Returns a tuple (Emitter, Receiver) denoting the end-points of the Connection or None, if any one of the
            two has expired
            """
            # check if the receiver is still alive
            receiver: Optional[Receiver] = self._receiver()
            if receiver is None:
                return None

            # check if the emitter is still alive
            emitter: Optional[Emitter] = self._emitter()
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

    def emit_value(self, emitter: 'Emitter', value: Any = Value()):
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

    def emit_failure(self, emitter: 'Emitter', error: Exception):
        """
        Schedules the failure of an Emitter in the Circuit.
        :param emitter: Emitter to fail.
        :param error:   Error causing the failure.
        """
        with self._event_condition:
            self._events.append(Circuit._FailureEvent(weak_ref(emitter), error))
            self._event_condition.notify_all()

    def emit_completion(self, emitter: 'Emitter'):
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
        """
        Creates a new Circuit Element and return it to the caller.
        :param element_type:    Type of Element to create.
        :returns:               New Circuit Element with a unique ID.
        """
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

    def create_connection(self, emitter: 'Emitter', receiver: 'Receiver'):
        """
        Schedules the addition of a connection in the Circuit.
        This method should only be called by a Receiver from within the Event loop.
        :param emitter:     Emitter at the source of the connection.
        :param receiver:    Receiver at the target of the connection.
        """
        self._topology_changes.append(Circuit._TopologyChange(weak_ref(emitter), weak_ref(receiver),
                                                              Circuit._TopologyChange.Kind.CREATE_CONNECTION))

    def remove_connection(self, emitter: 'Emitter', receiver: 'Receiver'):
        """
        Schedules the removal of a connection in the Circuit.
        This method should only be called by a Receiver from within the Event loop.
        :param emitter:     Emitter at the source of the connection.
        :param receiver:    Receiver at the target of the connection.
        """
        self._topology_changes.append(Circuit._TopologyChange(weak_ref(emitter), weak_ref(receiver),
                                                              Circuit._TopologyChange.Kind.REMOVE_CONNECTION))

    def expire_emitter(self, emitter: 'Emitter'):
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
            connection: Optional[Tuple[Emitter, Receiver]] = change.get_endpoints()
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

    def _create_connection(self, emitter: 'Emitter', receiver: 'Receiver'):
        """
        Creates a new connection in the Circuit during the event epilogue.
        :param emitter: Emitter at the source of the connection.
        :param receiver: Receiver at the target of the connection.
        """
        # never create a connection mid-event
        assert emitter._status in (Emitter._Status.IDLE,
                                   Emitter._Status.FAILED,
                                   Emitter._Status.COMPLETED)

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

    def _remove_connection(self, emitter: 'Emitter', receiver: 'Receiver'):
        """
        Removes a connection in the Circuit during the event epilogue.
        :param emitter: Emitter at the source of the connection.
        :param receiver: Receiver at the target of the connection.
        """
        if emitter in receiver._upstream:
            receiver._upstream.remove(emitter)

            # mark the emitter as a candidate for deletion
            self.expire_emitter(emitter)

        weak_receiver: weak_ref = weak_ref(receiver)
        if weak_receiver in emitter._downstream:
            emitter._downstream.remove(weak_ref(receiver))


########################################################################################################################

from .emitter import Emitter
from .receiver import Receiver
from .signals import CompletionSignal
