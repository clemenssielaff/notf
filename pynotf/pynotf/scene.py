from typing import Any, Dict, Optional, List, Tuple, NamedTuple, Type
from threading import Thread, Lock
from abc import abstractmethod
import asyncio
from inspect import iscoroutinefunction
from functools import partial
from enum import Enum, auto
from logging import error as print_error
from traceback import format_exc
from uuid import uuid4 as uuid
from weakref import ref as weak_ref

from .value import Value
from .logic import Publisher, Operator


########################################################################################################################

class HasNode:
    """
    Interface class for subclasses of Subscriber that live on a Node.
    By having this common interface, Publishers that may want to sort their Subscribers during a publish can identify
    those that are connected to a Node and can ask them for it.
    """

    def __init__(self, node: 'Node'):
        """
        Constructor.
        :param node: Reference to the Node that this Subscriber lives on.
        """
        self._node: Node = node

    @property
    def node(self) -> 'Node':
        """
        The Node that this Subscriber lives on.
        """
        return self._node


########################################################################################################################

class Property(Operator, HasNode):
    """
    Properties are values that combined fully define the state of the Node.
    If you take the entire Scene, take all Properties of all the Nodes, serialize the Scene alongside the Properties and
    re-load them, you have essentially restored the entire UI application at the point of serialization. Each Property
    is an operator that produces a single Value.
    The Operations that define how a Property transforms its input Value are fixed as they are an essential part of the
    Property type.
    """

    def __init__(self, node: 'Node', value: Value, *operations: Operator.Operation):
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
            # we need at least one Operator store provide Value Schemas for input and output
            operations = (Operator.NoOp(self._value.schema),)

        Operator.__init__(self, *operations)
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
                self.publish(self._value)

    def on_next(self, publisher: Publisher, value: Optional[Value] = None):
        """
        Abstract method called by any upstream publisher.

        :param publisher    The Publisher publishing the value, for identification purposes only.
        :param value        Published value, can be None (but this will throw in error for Properties).
        """
        self.value = value

    def on_error(self, publisher: Publisher, exception: Exception):
        pass  # Properties do not care about upstream errors

    def complete(self):
        pass  # Properties cannot be completed manually


########################################################################################################################

class Slot(Operator, HasNode):
    """
    A Slot is just a Relay that has an associated Node.
    """

    def __init__(self, node: 'Node', schema: Value.Schema = Value.Schema()):
        """
        Constructor.
        :param node: Node that this Slot lives on.
        :param schema: Schema of the Value accepted by this Slot.
        """
        Operator.__init__(self, Operator.NoOp(schema))
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

    Signals are Publishers that live on the Node and than can be invoked to propagate non-Property Values into the
    Logic as needed. They are public, meaning that they can be triggered from everywhere, although most will probably
    be triggered from code living on the Node itself.

    Slots are Subscribers that live on the Node and keep a reference to their Node. Publishers that sort their
    Subscribers based on their Node (if they have one) make use of that reference to establish an order for each new
    Publication.

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
            operations: Tuple[Operator.Operation] = tuple()

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

        def add_property(self, name: str, value: Any, *operations: Operator.Operation):
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
            :param schema: Scheme of the Value published by the Signal.
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

    def __init__(self, scene: 'Scene', parent: 'Node', name: Optional[str] = None):
        """
        Constructor.
        :param scene: The Scene that this Node lives in.
        :param parent: The parent of this Node.
        :param name: (Optional) Scene-unique name of the Node.
        :raise NameError: If another Node with the same (not-None) name is alive in the Scene.
        """
        self._scene: Scene = scene
        self._parent: Node = parent
        self._children: List[Node] = []
        self._properties: Dict[str, Property] = {}
        self._signals: Dict[str, Publisher] = {}
        self._slots: Dict[str, Slot] = {}
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

    def get_signal(self, name: str) -> Publisher:
        """
        Returns the Signal requested by name.
        :raises KeyError: If this Node kind has no Signal by the given name.
        """
        signal: Optional[Publisher] = self._signals.get(name, None)
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

    def create_child(self, node_type: Type['Node'], name: Optional[str] = None) -> 'Node':
        """
        Create a new child Node of this Node.
        By tying Node creation to the existence of its parent, we can make sure that a Node can never not have a valid
        parent. We can also call `_produce_definition` on the new instance after is has been constructed.
        :param node_type: Subclass of Node to instantiate as a child of this Node.
        :param name: (Optional) Scene-unique name of the new Node.
        :return: The new child Node instance.
        """
        if not issubclass(node_type, Node):
            raise TypeError("Nodes can only have other Nodes as children")

        # create the node instance and define it
        node = node_type(self._scene, self, name)
        node._apply_definition(node._produce_definition())

        # register the node as a new child of this one
        self._children.append(node)

        return node

    @abstractmethod
    def _produce_definition(self) -> 'Node.Definition':
        """
        Node subclasses should not be able to create Properties, Signals or Slots. Therefore, they are no methods like
        `_add_property` etc. However, they still need a way to define the Properties, Signals and Slots what they need
        in order to function.
        This abstract method must be overwritten by a Node subtype and produce a `Definition` object that will be used
        to define the instance of the new Node upon creation in `Node.create_child`.
        :return: Node Definition object to define Nodes of the specific sub-type.
        """
        raise NotImplementedError()

    def _apply_definition(self, definition: 'Node.Definition'):
        """
        This method cannot be called in the constructor, since (in general, meaning C++) you cannot call virtual methods
        inside the constructor. And we need to call `_produce_definition` in order to get the Definition object to apply
        to this Node instance in the first place. This is why this method is instead called by `Node.create_child`.
        :param definition: Node Definition object to define this Node instance with.
        """
        self._properties = {name: Property(self, prop.value, *prop.operations)
                            for name, prop in definition.properties.items()}
        self._signals = {name: Publisher(signal.schema) for name, signal in definition.signals.items()}
        self._slots = {name: Slot(self, slot.schema) for name, slot in definition.slots.items()}

    # TODO: remove Nodes


########################################################################################################################

class RootNode(Node):
    """
    The Root Node is its own parent and has the reserved name '/'.
    """

    def __init__(self, scene: 'Scene'):
        Node.__init__(self, scene, self, '/')

    def _produce_definition(self):
        """
        Root Nodes have no Properties, Signals or Slots.
        This method should never be called because the only way to get is is to try to add a Root Node as a child of
        an existing Node.
        """
        raise RuntimeError("RootNodes cannot be a child of another Node")


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
        self._named_nodes: Dict[str: weak_ref] = {}
        self._root: RootNode = RootNode(self)

    @property
    def root(self) -> RootNode:
        """
        The Root Node of this Scene.
        """
        return self._root

    def get_node(self, name: str) -> Optional[Node]:
        """
        Looks for a Node with the given name and returns it if one is found.
        :param name: Name of the Node to look for.
        """
        weak_node: weak_ref = self._named_nodes.get(name, None)
        return None if weak_node is None else weak_node()

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

    def __init__(self):
        """
        Default constructor.
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

        # Inject a call to `self._cleanup` every time the event loop has run
        # This is a dirty hack that relies on an implementation detail of CPython, but since this is proof-of-concept
        # code only, I don't care.
        self._loop._run_once = lambda: asyncio.BaseEventLoop._run_once(self._loop) or self._cleanup()

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
        Since the order in which the Logic publishes values is essentially random, we cannot outright delete Nodes when
        they are first marked for removal. Instead, we need to remove them after the loop has finished a turn and no
        change is currently handled.
        This method does that.
        """
        pass  # TODO: Executor._cleanup

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


########################################################################################################################

class Fact(Publisher):
    """
    Facts represent changeable, external truths that the Application Logic can react to.
    To the Application Logic they appear as simple Publishers that are owned and managed by a single Service in a
    thread-safe fashion. The Service will update the Fact as new information becomes available, completes the Fact's
    Publisher when the Service is stopped or the Fact has expired (for example, when a sensor is disconnected) and
    forwards appropriate exceptions to the Subscribers of the Fact should an error occur.
    Examples of Facts are:
        - the latest reading of a sensor
        - the user performed a mouse click (incl. position, button and modifiers)
        - the complete chat log
        - the expiration signal of a timer
    Facts consist of a Value, possibly the empty None Value. Empty Facts simply informing the logic that an event has
    occurred without additional information.
    """

    def __init__(self, executor: Executor, schema: Value.Schema = Value.Schema()):
        Publisher.__init__(self, schema)
        self._executor: Executor = executor

    def publish(self, value: Any = Value()):
        """
        Push the given value to all active Subscribers.
        :param value: Value to publish, can be empty if this Publisher does not publish a meaningful value.
        """
        self._executor.schedule(Publisher.publish, self, value)

    def _error(self, exception: Exception):
        """
        Failure method, completes the Publisher.
        :param exception:   The exception that has occurred.
        """
        self._executor.schedule(Publisher._error, self, exception)

    def _complete(self):
        """
        Completes the Publisher successfully.
        """
        self._executor.schedule(Publisher._complete, self)


########################################################################################################################

"""
Ideas
=====

Rename Publisher to Signal?
--------------------------
A Signal is a Publisher, but the word "Publisher" feels wrong when talking about a Signal. However, a Signal could well
be a term used for a Publisher as well. I guess "Subscriber" would then be an "Observer"? A "Slot" would still be its
own term, since it also implements the "HasNode"-interface.
Of course, the thing that is published by a Signal cannot itself be named "Signal"...


Additional Thoughts
===================


Is a Widget a Node?
-------------------
The way thing are set up now, we could turn the Widget -> Node relationship from an "is-a" to a "has-a" relationship.
Meaning instead Widget deriving from Node, we could simply contain a Node as a field in Widget. Usually this is
preferable, if not for one thing: discoverability. What I mean is that you can find a Node using its name in a Scene,
but if the Node knows nothing about Widgets (or potential other Node-subclasses), then you don't get to a Widget from
a Node. Instead, when a Widget *is-a* Node, the Node class can still be kept in the dark about the existence of a Widget
class, but you will be able to `dynamic_cast` from the Node to a Widget.


Asynchronous Operations
-----------------------
Many languages allow await/async syntax, Python is among them. I want to do something similar in the general case,
using fibers in C++. Where in the Logic graph do we allow for asynchronous code to run? One easy answer is to put them 
behind dedicated "async-operations", that take a value + Signal and spawn a new coroutine that can be suspended while 
waiting on some asynchronous event to take place. This leads to follow up questions:
    1.) Do we switch to new coroutines immediately and run them until they halt? Or Are they scheduled to run 
        immediately after the non-concurrent code has finished?
    2.) How are Signals accepted or blocked with async code?
Actually, those two questions sound like the same one. If we only have Publishers that do not care about the order in 
which their Subscribers get updated, then it shouldn't matter whether an asynchronous subscriber is called immediately
or after all other subscribers have been called. It might actually even be better to delay all asynchronous subscribers
until after all synchronous ones have been handled, because async means "now or later" while synchronous means "now" and
scheduling all coroutines for later would honor that meaning more than those that execute immediately.

Therefore, a sync code sits behind async operators in the logic. Async operators ignore the upstream Signal and 
propagate an unblockable downstream Signal, meaning accepting or blocking has to happen before. Publications downstream 
of the async operator are scheduled to happen after the synchronous "parent" Publication.

It should never be acceptable to accept or block a publication in async code - if we'd allow that, then it would be 
possible for an event to be delayed indefinitely by a sibling subscriber.
Example: A mouse click is delivered to two unrelated widgets that happen to overlap on screen. If the top widget 
receives the publication and waits for three seconds to decide whether to accept it or not, the bottom widget will do 
nothing and only be able to respond after the three seconds have passed. This might be what you want in some weird edge 
case, but generally accept/ ignore/block decisions should be instantaneous.

Interestingly this mirrors the way Python handles its own async code: You have to mark async functions with the async 
keyword and only then can you await.


Widgets and Logic
-----------------
We have now established that Logic elements (Publisher and Subscribers) must be protected from removal while an update
is being handled. The update process is also protected from the unexpected addition of elements, but that does not 
matter for the following question, which is about the relationship between logic elements and Widgets.

First off, Widgets own their logic elements. This is elementary and one of the features of notf: a Widget is fully
defined by the combined state of its Properties, while Signals and Slots are tied to the type of Widget like methods are
to an C++ object. But do some logic elements also own their Widget? Properties should not, they are basically dumb 
values that exist in the context of a Widget but are fully unaware of it. Signals do not, they are outward facing only
and do not carry any additional state beyond that of what any Publisher has.
Slots however can (and often will) require a strong reference to the Widget that they live on. What now if we face
problem 1 from the chapter "Logic Modifications" and Widget B removes Widget C and vice-versa? 

It is clear that an immediate removal is out of the question (see above). We still need to remove them, but only when it
is absolutely safe to do so. And the only time during an update process when it is safe to do so is at the very end.
At the moment, the Widget is flagged for deletion, a strong reference to it is added to a set of "to delete" Widgets in
the Graph. Then, at the very end of the update, we discard every Widget that has an ancestor in the "to delete" set and
delete the rest. This way, we keep the re-layouting to a minimum. 

Note that it is not possible to check whether a Widget is going to be removed at the end of the update process or not,
even though such a method would be easy enough to implement. The problem is the same as with an immediate removal. 
Depending on the execution order, the flag of Widget A would either say "this Widget is going to be deleted" if B 
was updated first or "this Widget is not scheduled for removal" if A is updated first. The result is not wrong, but
essentially useless.
The same goes for anything else that might be used as "removal light", like removing the Widget as child of the parent
so it does not get found anymore when the hierarchy is traversed.


Properties
----------
Widget Properties are Operators. Instead of having a fixed min- or max or validation function, we can simply define them
as Operators with a list of Operations that are applied to each new input.


Dead Ends
========= ==============================================================================================================
*WARNING!* 
The chapters below are only kept as reference of what doesn't make sense, so that I can look them up once I re-discover 
the "brilliant" idea underlying each one. Each chapter has a closing paragraph on why it actually doesn't work.


Multithreaded evaluation
------------------------
The topic of true parallel execution of the Application Logic has been a difficult one. On the surface it seems like
it should be possible - after all, what is the Logic but a DAG? And parallel graph execution has been done countless 
times. The problem however is twofold:
    1. Logic operations are allowed to add, move or remove other operations in the Logic graph.
    2. Since we allow user-defined code to execute in the Logic (be that dynamically through scripting or statically 
       at compile time), we cannot be certain what dependencies an operation has.
Actually, looking at the problems - it is really just one: User-defined code. 

This all means that it is impossible to statically determine the topology of the graph. Even if you propagate an Event 
once, you cannot be certain that the same Event with a different Value would reach the same child operations, since any
Operation could contain a simply if-statement for any specific case which would be transparent from the outside. And
event with the same Value you might get a different result, as long as the user defined code is allowed to store its own
state.
Without a fixed topology, you do not have multithreaded execution.

However ...  
There is this idea called "Event Sourcing" (https://martinfowler.com/eaaDev/EventSourcing.html) that basically says that
instead of storing the state of something, we only store all the events that modify that something and then, if someone
asks, use the events to generate the state that we need. For example, instead of storing the amount of money in the bank
account, we assume that it starts with zero and only store transactions. Then, if you want to look up your current
balance, we replay all transactions and generate the answer.
That alone does not help here though. Our problem is not the state of the Logic, but its lack of visibility. We only
know which Logic operators were read, written, created or destroyed *after* the event has been processed. Since there
is no way to know that in advance, we cannot schedule multiple threads executing the Logic in parallel without creating
race conditions.
But what if we keep a record of ever read, write, creation or destruction that ever event triggered? This will not solve
the problems of racy reads and writes, but it will allow us to reason about dependencies of events after the fact.
So if we have event A and B (in that order), we handle both in parallel (based on the Logic as it presented itself at
the start of the event loop) and afterwards find out that they touched completely different operators? We do nothing.
There was no possible race condition and we can update the Logic using the CRUD (create, read, update, delete) records
that both Events generated independently. If however we find an Operator that:
    * A and B wrote to
    * A wrote to and B read
    * A destroyed and B read
we must assume that the records from B are invalid, as it was run on an outdated Logic graph. Now we need to re-run B, 
after applying the changes from A. 
And yes, that does mean that we might have to do some work twice. It also means that we  have to keep track of child 
events, so they too can be destroyed should their parent (B in the example) be found out to be invalid; otherwise B 
might spawn invalid and/or duplicate child events. But on the plus side, if enough events are independent (which I 
assume they will be) we can have true parallel execution for most Events.
Whether or not this is actually useful is left to be determined through profiling.

Okay, counterpoint: Since we have user-defined code; *state-full* user-defined code, how can we guarantee that the 
effect of an Event can be truly reversible? Meaning, if we do find that B is invalid, how can we restore the complete
state of the Logic graph to before B was run? "complete" being the weight-bearing word here. The point is: we can not.
Not, as long as the user is able to keep her own state around that we don't know anything about.
The true question is: can we allow that? Or asked differently, do we gain enough by cutting the user's ability to 
manage her own state? 
Probably not. The dream of true parallel Logic execution remains as alluring as it is elusive. It sure sounds good to 
say that all events are executed in parallel, but that might well be a burden. It is not unlikely that most events will 
be short or even ultra-short lived (by that I mean that they are published straight into Nirvana, without a single 
Subscriber to react) and opening up a new thread for each one would be a massive overhead. On top of that, you'd have to
record CRUD and analyse the records - more overhead. And we didn't even consider the cost of doing the same work twice 
if a collision in the records is found. Combine that with the fact that we would need to keep track of *all* of the 
state and must forbid the user to manage any of her own ... I say it is not worth it.


Property Visibility
-------------------
In previous versions, Node Properties had a flag associated with their type called "visibility". A visible Property was
one that, when changed, would update the visual representation of the Widget and would cause a redraw of its Design.
I chose to get rid of this in favor of a more manual approach, where you have to call "redraw" in a Widget manually.
Here is why the visibility idea was not good:
    - Visibility only makes sense for Widgets, but all Nodes have Properties. Therefore all Nodes are tied to the 
      concept of visibility even though it only applies to a sub-group.
    - Sometimes, whether or not the Design changes is not a 1:1 correspondence on a Property update. For example, some
      Properties might only affect the Design if they change a certain amount, or if they are an even number... or
      whatever.
"""
