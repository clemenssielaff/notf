from typing import Any, Dict, Optional, List, Type, Tuple, NamedTuple, TypeVar, Set
from threading import Thread, Lock
import asyncio
from inspect import iscoroutinefunction
from functools import partial
from enum import Enum, auto
from logging import error as print_error
from traceback import format_exc
from uuid import uuid4 as uuid
from weakref import ref as weak_ref
from gc import collect as force_gc

from .value import Value
from .logic import Emitter, Switch

NodeType = TypeVar('NodeType')




########################################################################################################################

class Property(Switch, HasNode):
    """
    Properties are values that combined fully define the state of the Node.
    If you take the entire Scene, take all Properties of all the Nodes, serialize the Scene alongside the Properties and
    re-load them, you have essentially restored the entire UI application at the point of serialization. Each Property
    is also a Switch that produces a single Value.
    The Operations that define how a Property transforms its input Value are fixed as they are an essential part of the
    Property type.
    """

    def __init__(self, node: 'Node', value: Value, *operations: Switch.Operation):
        """
        Constructor.
        :param node: Node that this Property lives on.
        :param value: Initial value of the Property, also defines its Schema.
        :param operation: Operations applied to every new Value of this Property, must ingest and produce Values of the
            Property's type. Can be empty.
        :raise TypeError: If the input of the first or the output of the last Operation does not match this Property's
            Value type.
        """
        self._value: Value = value

        if len(operations) == 0:
            # we need at least one Switch store provide Value Schemas for input and output
            operations = (Switch.NoOp(self._value.schema),)

        Switch.__init__(self, *operations)
        HasNode.__init__(self, node)

        if self.input_schema != self.output_schema:
            raise TypeError("Property Operations must not change the type of Value")
        if self.input_schema != self._value.schema:
            raise TypeError("Property Operations must ingest and produce the Property's Value type")

    @property
    def value(self) -> Value:
        """
        The current value of this Property.
        """
        return self._value

    @value.setter
    def value(self, value: Any):
        """
        Applies a new Value to this Property. The Value must be of the Property's type and is transformed by the
        Property's chain of Operations before assignment.
        :param value: New value to assign to this Property.
        :raise TypeError: If the given Value type does not match the one stored in this Property.
        """
        if value is None:
            raise TypeError("Property values cannot be none")

        if not isinstance(value, Value):
            try:
                value = Value(value)
            except ValueError as exception:
                raise TypeError(str(exception)) from exception
        assert isinstance(value, Value)

        if value.schema != self._value.schema:
            raise TypeError("Cannot change the type of a Property")

        try:
            result = self._operate_on(value)
        except Exception as exception:
            self._error(exception)
        else:
            if result != self._value:
                self._value = result
                self.emit(self._value)

    def on_value(self, emitter: Emitter, value: Optional[Value] = None):
        """
        Abstract method called by any upstream Emitter.

        :param emitter  The Emitter emitting the value, for identification purposes only.
        :param value    Emitted value, can be None (but this will throw in error for Properties).
        """
        self.value = value

    def on_error(self, emitter: Emitter, exception: Exception):
        pass  # Properties do not care about upstream errors

    def complete(self):
        pass  # Properties cannot be completed manually


########################################################################################################################

class Slot(Switch, HasNode):
    """
    A Slot is just a Relay that has an associated Node.
    """

    def __init__(self, node: 'Node', schema: Value.Schema = Value.Schema()):
        """
        Constructor.
        :param node: Node that this Slot lives on.
        :param schema: Schema of the Value accepted by this Slot.
        """
        Switch.__init__(self, Switch.NoOp(schema))
        HasNode.__init__(self, node)


########################################################################################################################

class Node:
    """
    Node is the base class for everything that depends on the state of the UI. Widgets are prime examples of Nodes as
    they are created and destroyed as the UI changes, but while Widgets are Nodes, not all Nodes are Widgets.

    Nodes consists of 3 elements:
        1. Properties
        2. Signals
        3. Slots

    Signals are Emitters that live on the Node and than can be invoked to propagate non-Property Values into the
    Logic as needed. They are public, meaning that they can be triggered from everywhere, although most will probably
    be triggered from code living on the Node itself.

    Slots are Receivers that live on the Node and keep a reference to their Node. Emitters that sort their Receivers
    based on their Node (if they have one) make use of that reference to establish an order for each new Signal.

    Node instances are uniquely identified by an UUID which persists even through serialization/deserialization.
    """

    class Definition:
        """
        Instead of implementing `_add_property`, `_add_signal` and `_add_slot` methods in the Node class, a Node must be
        fully defined on construction, by passing in a Node.Definition object. Like a Schema describes a type of Value,
        the Node.Definition describes a type of Node without making any assumptions about the actual state of the Node.

        Properties and Slots must be constructed with a reference to the Node instance that they live on. Therefore,
        we only store the construction arguments here and build the object in the Node constructor.
        """

        class PropertyArgs(NamedTuple):
            value: Value
            operations: Tuple[Switch.Operation] = tuple()

        class SignalArgs(NamedTuple):
            schema: Value.Schema = Value.Schema()

        class SlotArgs(NamedTuple):
            schema: Value.Schema = Value.Schema()

        def __init__(self):
            """
            Empty constructor.
            """
            self._properties: Dict[str, Node.Definition.PropertyArgs] = {}
            self._signals: Dict[str, Node.Definition.SignalArgs] = {}
            self._slots: Dict[str, Node.Definition.SlotArgs] = {}

        @property
        def properties(self):
            return self._properties

        @property
        def signals(self):
            return self._signals

        @property
        def slots(self):
            return self._slots

        def add_property(self, name: str, value: Any, *operations: Switch.Operation):
            """
            Stores the arguments need to construct a Property instance on the node.
            :param name: Node-unique name of the Property.
            :param value: Initial value of the Property.
            :param operations: Operations applied to every new Value of this Property.
            :raise NameError: If the Node already has a Property, Signal or Slot with the given name.
            """
            if not self._is_name_available(name):
                raise NameError("Node already has a Property named {}".format(name))
            self._properties[name] = Node.Definition.PropertyArgs(value, operations)

        def add_signal(self, name: str, schema: Value.Schema = Value.Schema()):
            """
            Stores the arguments need to construct a Signal instance on the node.
            :param name: Node-unique name of the Signal.
            :param schema: Scheme of the Value emitted by the Signal.
            :raise NameError: If the Node already has a Property, Signal or Slot with the given name.
            """
            if not self._is_name_available(name):
                raise NameError("Node already has a Signal named {}".format(name))
            self._signals[name] = Node.Definition.SignalArgs(schema)

        def add_slot(self, name: str, schema: Value.Schema = Value.Schema()):
            """
            Stores the arguments need to construct a Slot instance on the node.
            :param name: Node-unique name of the Slot.
            :param schema: Scheme of the Value received by the Slot.
            :raise NameError: If the Node already has a Property, Signal or Slot with the given name.
            """
            if not self._is_name_available(name):
                raise NameError("Node already has a Signal named {}".format(name))
            self._slots[name] = Node.Definition.SlotArgs(schema)

        def _is_name_available(self, name: str) -> bool:
            """
            Properties, Signals and Slots share the same namespace on a Node. Therefore we need to check whether any
            one of the three dictionaries already contains a given name before it can be accepted as new.
            :param name: Name to test
            :return: Whether or not the given name would be a valid new name for a Property, Signal or Slot.
            """
            return not ((name in self._properties) or (name in self._signals) or (name in self._slots))

    def __init__(self, parent: 'Node', definition: 'Node.Definition', name: Optional[str] = None):
        """
        Constructor.
        :param parent: The parent of this Node.
        :param definition: Node Definition of this Node subclass.
        :param name: (Optional) Scene-unique name of the Node.
        :raise NameError: If another Node with the same (not-None) name is alive in the Scene.
        """
        self._parent: Node = parent
        self._scene: Scene = self._parent._scene
        self._children: List[Node] = []

        self._properties: Dict[str, Property] = {name: Property(self, prop.value, *prop.operations)
                                                 for name, prop in definition.properties.items()}
        self._signals: Dict[str, Emitter] = {name: Emitter(signal.schema) for name, signal in
                                             definition.signals.items()}
        self._slots: Dict[str, Slot] = {name: Slot(self, slot.schema)
                                        for name, slot in definition.slots.items()}

        self._uuid: uuid = uuid()
        self._name: Optional[str] = name  # in C++ this could be a raw pointer to the name inside the Scene's registry

        if self._name is not None:
            self._scene.register_name(self)

    @property
    def name(self) -> Optional[str]:
        """
        The Scene-unique name of this Node if it has any.
        """
        return self._name

    @property
    def uuid(self) -> uuid:
        """
        Universally unique ID of this Node.
        """
        return self._uuid

    @property
    def parent(self) -> 'Node':
        """
        The parent of this Node.
        """
        return self._parent

    def get_property(self, name: str) -> Property:
        """
        Returns the Property requested by name.
        :raises KeyError: If this Node kind has no Property by the given name.
        """
        prop: Optional[Property] = self._properties.get(name, None)
        if prop is None:
            raise KeyError("Node has no Property named: {}. Available Property names are: {}".format(name, ", ".join(
                self._properties.keys())))
        return prop

    def get_signal(self, name: str) -> Emitter:
        """
        Returns the Signal requested by name.
        :raises KeyError: If this Node kind has no Signal by the given name.
        """
        signal: Optional[Emitter] = self._signals.get(name, None)
        if signal is None:
            raise KeyError("Node has no Signal named: {}. Available Signal names are: {}".format(name, ", ".join(
                self._signals.keys())))
        return signal

    def get_slot(self, name: str) -> Slot:
        """
        Returns the Slot requested by name.
        :raises KeyError: If this Node kind has no Slot by the given name.
        """
        slot: Optional[Slot] = self._slots.get(name, None)
        if slot is None:
            raise KeyError("Node has no Slot named: {}. Available Slot names are: {}".format(name, ", ".join(
                self._slots.keys())))
        return slot

    def create_child(self, node_type: Type[NodeType], name: Optional[str] = None) -> NodeType:
        """
        Create a new child Node of this Node.
        By tying Node creation to the existence of its parent we can make sure that a Node always has a valid parent.
        :param node_type: Subclass of Node to instantiate as a child of this Node.
        :param name: (Optional) Scene-unique name of the new Node.
        :return: The new child Node instance.
        """
        if not issubclass(node_type, Node):
            raise TypeError("Nodes can only have other Nodes as children")

        # create the node instance and define it
        node = node_type(self, name)

        # register the node as a new child of this one
        self._children.append(node)

        return node

    def remove(self):
        """
        Schedules this Node to be removed at the end of the next loop.
        """
        self._scene._expired_nodes.add(self)


########################################################################################################################

class RootNode(Node):
    """
    The Root Node is its own parent and has the reserved name '/'.
    """

    def __init__(self, scene: 'Scene'):
        self._scene: Scene = scene
        """
        This is a bit of a hack. A Node copies its Scene reference from its parent, but since the 
        root is its own parent, we need to make sure that it has the Scene when it asks itself for 
        it.
        """

        Node.__init__(self, self, Node.Definition(), '/')


########################################################################################################################

class Scene:
    """
    Nodes are stored in a hierarchy, called "the Scene". In the Scene, parent Nodes own their child Nodes with every
    child having exactly one parent. Although the parent can change, it can never be empty.
    If a Node's parent is  itself,  it is a root Node. There should be only one root Node most of the time, but in case
    you want to completely  replace the entire hierarchy (for example when loading / hot-reloading an application) it
    must be possible to have more than one root.
    Children are stored in a list, meaning they are ordered. The meaning of the order depends on the type of Node, for
    Widgets later children are drawn on top of earlier ones.

    Modification of the Scene must only occur in the Logic thread.

    Nodes can be named. They have a name associated with them during construction and it can not be changed afterwards.
    Names are unique in the Scene, if you try to create two Nodes with the same name, the creation of the second one
    will fail.
    Root Nodes have the reserved name '/', we can therefore be sure that only one Root Node can exist in the Scene.
    """

    def __init__(self):
        """
        Constructor.
        """
        self._named_nodes: Dict[str: weak_ref] = dict()
        self._root: RootNode = RootNode(self)
        self._expired_nodes: Set[Node] = set()

    def register_name(self, node: Node):
        """
        Registers a new Node with its name. If the Node does not have a name, this method does nothing.
        Must only be called from the Node constructor.
        :param node: Named Node to register.
        :raise NameError: If another Node with the same name is alive in the Scene.
        """
        name = node.name
        if name is None:
            return

        weak_node = self._named_nodes.get(name, None)
        existing_node = None if weak_node is None else weak_node()
        if existing_node is None:
            self._named_nodes[name] = weak_ref(node)
        elif existing_node != node:
            raise NameError('Cannot create another Node with the name "{}"'.format(name))

    def remove_expired_nodes(self) -> bool:
        """
        Is called at the end of the event loop by an Executor to remove all expired Nodes.
        :return: True iff any Nodes were removed.
        """
        if len(self._expired_nodes) == 0:
            return False

        for node in self._expired_nodes:
            node.parent._children.remove(node)
        self._expired_nodes.clear()
        return True


########################################################################################################################

class Executor:
    """
    The Application logic runs on a separate thread that is further split up into multiple fibers. In Python, we use the
    asyncio module for this, in C++ this would be boost.fiber. Regardless, we need a mechanism for cooperative
    concurrency (not parallelism) that makes sure that only one path of execution is active at any time.
    It is the executor's job to provide this functionality.
    """

    class _Status(Enum):
        """
        Status of the Executor with the following state transition diagram:
            --> Running --> Stopping --> Stopped
                  |                        ^
                  +------------------------+
        """
        RUNNING = 0
        STOPPING = auto()
        STOPPED = auto()

    def __init__(self, scene: Scene):
        """
        Constructor.
        :param scene: Scene to manage.
        """
        self._scene: Scene = scene
        """
        Scene managed by this Executor.
        """

        self._loop: asyncio.AbstractEventLoop = asyncio.new_event_loop()
        """
        Python asyncio event loop, used to schedule coroutines.
        """

        self._thread: Thread = Thread(target=self._run)
        """
        Logic thread, runs the Logic's event loop.
        """

        self._status = self._Status.RUNNING
        """
        Used for managing the Executor shutdown process. As soon as the `_stop` coroutine (in Executor.stop) is 
        scheduled, the status switches from RUNNING to either STOPPING or STOPPED. Only an Executor in the RUNNING
        state is able to schedule new coroutines.
        The difference between STOPPING and STOPPED is that a STOPPING Executor will wait until all of its scheduled
        coroutines have finished, whereas a STOPPED Executor will cancel everything that is still waiting and finish
        the Logic thread as soon as possible. 
        """

        self._mutex: Lock = Lock()
        """
        Mutex protecting the status, in C++ an atomic value would do.
        """

        # start the logic thread immediately
        self._thread.start()

    def schedule(self, func, *args, **kwargs):
        """
        Schedule a new function or coroutine to run if the logic is running.

        :param func: Function to execute in the logic thread.
        :param args: Variadic arguments for the function.
        :param kwargs: Keyword arguments for the function.
        """
        with self._mutex:
            if self._status is not self._Status.RUNNING:
                return
        self._schedule(func, *args, **kwargs)

    def _schedule(self, func, *args, **kwargs):
        """
        Internal scheduling, is thread-safe.

        :param func: Function to execute in the logic thread.
        :param args: Variadic arguments for the function.
        :param kwargs: Keyword arguments for the function.
        """
        if iscoroutinefunction(func):
            self._loop.call_soon_threadsafe(partial(self._loop.create_task, func(*args, **kwargs)))
        else:
            self._loop.call_soon_threadsafe(partial(func, *args, **kwargs))

        # always clean up after yourself
        self._loop.call_soon_threadsafe(self._cleanup)

    def finish(self):
        """
        Finish all waiting coroutines and then stop the Executor.
        """
        self.stop(force=False)

    def stop(self, force=True):
        """
        Stop the Executor, optionally cancelling all waiting coroutines.
        :param force: If true (the default), all waiting coroutines will be cancelled.
        """
        with self._mutex:
            if self._status is not self._Status.RUNNING:
                return

            if force:
                self._status = self._Status.STOPPED
            else:
                self._status = self._Status.STOPPING

        async def _stop():
            self._loop.stop()

        self._schedule(_stop)

        self._thread.join()

    @staticmethod
    def exception_handler():
        """
        Exception handling for exceptions that escape the loop.
        Prints the exception stack trace.
        Is a public method so it can be monkey-patched at runtime (e.g. for testing).
        """
        print_error(
            "\n==================================\n"
            f"{format_exc()}"
            "==================================\n")

    def _cleanup(self):
        """
        Since the order in which the Logic emits values is essentially random, we cannot outright delete Nodes when
        they are first marked for removal. Instead, we need to remove them after the loop has finished a turn and no
        change is currently handled.
        This method does that.
        """
        if self._scene.remove_expired_nodes():
            # We really need those Nodes gone, and their signals, slots and properties with them.
            # However, many Nodes own Receivers that in turn own references back to the Node, creating
            # cycles. Those cycles keep the Node and all of its dependents around until the gc comes
            # around to collect them. We need to do that here.
            force_gc()

    def _run(self):
        """
        Logic thread code.
        """
        self._loop.set_exception_handler(lambda *args: self.exception_handler())
        asyncio.set_event_loop(self._loop)

        try:
            self._loop.run_forever()
        finally:
            pending_tasks = [task for task in asyncio.Task.all_tasks() if not task.done()]
            with self._mutex:
                cancel_all = self._status is self._Status.STOPPED
                self._status = self._Status.STOPPED
            if cancel_all:
                for task in pending_tasks:
                    task.cancel()

            # noinspection PyBroadException
            try:
                self._loop.run_until_complete(asyncio.gather(*pending_tasks))
            except asyncio.CancelledError:
                pass
            except Exception:
                self.exception_handler()
            finally:
                self._loop.close()

