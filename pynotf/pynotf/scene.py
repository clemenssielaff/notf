from typing import Any, Dict, Optional
from threading import Thread, Lock
import asyncio
from inspect import iscoroutinefunction
from functools import partial
from enum import Enum, auto
from logging import error as print_error, warning as print_warning
from traceback import format_exc

from .value import Value
from .logic import Publisher
from .operation import Transformer, Operation, NoOp


# from uuid import uuid4 as uuid
# from enum import Enum, unique, auto
# from logging import warning
# from typing import NamedTuple, List, TypeVar, Type, Union, Optional, Callable, Dict, Any
# from .reactive import Operator, Publisher
# from .value import Value
#
#
# class Property(Operator):
#
#     def clean(self):
#         assert is_this_the_main_thread()
#         if self._modified_value:
#             self._render_value = self._modified_value
#             self._modified_value = None
#

#
#
# class Node:
#     T = TypeVar('T')
#
#     class Data(NamedTuple):
#         parent: 'Node'  # mandatory
#         children: List['Node'] = []
#         is_enabled: bool = True
#         is_visible: bool = True
#         is_dirty: bool = True
#
#     def __init__(self, graph: 'Graph', parent: 'Node'):
#         self._graph: Graph = graph
#         self.__data: 'Node.Data' = self.Data(parent)
#         self._uuid: uuid = uuid()
#         self._properties: Dict[str, Property] = {}
#         # TODO: signals and slots
#
#     def _define_node(self, definition: Definition):
#         """
#         Is called right after the constructor has finished.
#         :param definition:  Node Definition. Is changed in-place to define the node instance.
#         """
#         raise NotImplementedError("Node subclasses must implement their virtual `_define_node` method")
#
#     def _apply_definition(self, definition: Definition):
#         """
#         Applies the given Node Definition to this instance.
#         :param definition: Node Definition to apply. Is constant.
#         """
#         self._properties = definition.properties
#         # TODO: automatic update when visible properties are updated
#
#     @property
#     def _data(self) -> Data:
#         """
#         A private getter for the Node data that can change depending on whether you ask for it from the main thread or
#         the render thread.
#         """
#         # TODO: render data & modified data
#         return self.__data
#
#     @property
#     def uuid(self) -> uuid:
#         return self._uuid
#
#     @property
#     def name(self) -> str:
#         return self._graph.registry.get_name(self)
#
#     @name.setter
#     def name(self, value: str):
#         self._graph.registry.set_name(self, value)
#
#     def create_child(self, node_type: Type['Node']) -> 'Node':
#         """
#         Create a new child Node of this Node.
#         By tying node creation to the existence of its parent node, we can make sure that a node can never not have a
#         valid parent. We can also call `_define_node` on the new instance.
#         :param node_type: Subclass of Node to instantiate as a child of this Node.
#         :return:          The new child Node instance.
#         """
#         # create the node instance
#         if not issubclass(node_type, Node):
#             raise TypeError("Nodes can only have other Nodes as children")
#         child = node_type(self._graph, self)
#
#         # finalize the node
#         definition = self.Definition()
#         child._define_node(definition)
#         child._apply_definition(definition)
#
#         # register the node as a new child
#         self._data.children.append(child)
#         return child
#
#     def property(self, name: str) -> Property:
#         """
#         Returns the Property requested by name.
#         :raises KeyError: If this Node kind has no Property by the given name.
#         """
#         if name in self._properties:
#             return self._properties[name]
#         raise KeyError("Node has no Property named: {}. Available Property names are: {}".format(name, ", ".join(
#             self._properties.keys())))
#
#
# class RootNode(Node):
#     """
#     The Root Node is its own parent.
#     """
#
#     def __init__(self, graph: 'Graph'):
#         super().__init__(graph, self)
#
#     def _define_node(self, definition: Node.Definition):
#         pass
#
#
# class Graph:
#     """
#     The UI Graph.
#     """
#
#     class NodeRegistry:
#         def __init__(self):
#             self._nodes: Dict[Union[Node, str], Union[Node, str]] = {}  # str <-> Node bimap
#
#         def __len__(self) -> int:
#             """
#             Number of node/name pairs in the registry.
#             """
#             assert len(self._nodes) % 2 == 0
#             return len(self._nodes) // 2
#
#         def get_name(self, node: Node) -> str:
#             name = self._nodes.get(node, None)
#             if name is None:
#                 # create a default name and register the node with it
#                 name = str(node.uuid)
#                 self._nodes[name] = node
#                 self._nodes[node] = name
#             return name
#
#         def set_name(self, node: Node, name: str) -> str:
#             # delete the old node/name pair if it exists
#             old_name = self._nodes.get(node, None)
#             if old_name is not None:
#                 assert node in self._nodes
#                 del self._nodes[old_name]
#                 del self._nodes[node]
#
#             # make sure the name is unique
#             proposal = name
#             counter = 1
#             while proposal in self._nodes:
#                 proposal = "{}{:>2}".format(name, counter)
#                 counter += 1
#
#             # (re-)register the node under its new name
#             self._nodes[proposal] = node
#             self._nodes[node] = proposal
#
#             # return the actual new name of the node
#             return proposal
#
#         def get_node(self, name: str) -> Optional[Node]:
#             return self._nodes.get(name, None)
#
#     def __init__(self):
#         self.root: RootNode = RootNode(self)
#         self.registry: Graph.NodeRegistry = Graph.NodeRegistry()


########################################################################################################################

class Property(Transformer):
    """
    Properties are values that combined fully define the state of the Node.
    If you take the entire Scene, take all Properties of all the Nodes, serialize the Scene alongside the Properties and
    re-load them, you have essentially restored the entire UI application at the point of serialization. Each Property
    is an operator that produces a single Value.
    The Operations that define how a Property transforms its input Value are fixed as they are an essential part of the
    Property type.
    """

    def __init__(self, name: str, value: Value, *operations: Operation):
        """
        Constructor.
        :param name: Node-unique name of the Property, is constant.
        :param value: Initial value of the Property, also defines its Schema.
        :param operation: Operations applied to every new Value of this Property, must ingest and produce Values of the
            Property's type. Can be empty.
        :raise TypeError: If the input of the first or the output of the last Operation does not match this Property's
            Value type.
        """
        self._name: str = name
        self._value: Value = value

        if len(operations) == 0:
            operations = (NoOp(self._value.schema),)

        Transformer.__init__(self, *operations)

        if self.input_schema != self.output_schema:
            raise TypeError("Property Operations must not change the type of Value")
        if self.input_schema != self._value.schema:
            raise TypeError("Property Operations must ingest and produce the Property's Value type")

    @property
    def name(self) -> str:
        """
        Node-unique name of this Property.
        """
        return self._name

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

        value = self._operator(value)

        if value == self._value:
            return
        self._value = value

        self.publish(value)

    def on_next(self, publisher: Publisher, value: Optional[Value] = None):
        """
        Abstract method called by any upstream publisher.

        :param publisher    The Publisher publishing the value, for identification purposes only.
        :param value        Published value, can be None (but this will throw in error for Properties).
        """
        self.value = value

    def on_error(self, publisher: Publisher, exception: BaseException):
        print_warning("Caught propagated error from Publisher {}: {}".format(publisher, str(exception)))

    def complete(self):
        pass  # Properties cannot be completed manually


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

        """

        def __init__(self):
            self._properties: Dict[str, Property] = {}

        @property
        def properties(self):
            return self._properties

        def add_property(self, name: str, value: Any, is_visible: bool) -> Property:
            """
            Creates and returns a new Property instance, that will be attached to the node.
            :param name: Node-unique name of the Property.
            :param value: Initial value of the Property.
            :param is_visible: Whether or not changes in this Property should cause a redraw of the Node or not.
            :return: New Property instance.
            """
            if name in self._properties:
                raise NameError("Node already has a property named {}".format(name))
            new_property = Property(name, value, is_visible)  # TODO: continue here
            self._properties[name] = new_property
            return new_property


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
    """

    def __init__(self):
        pass


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

    def error(self, exception: Exception):
        """
        Failure method, completes the Publisher.
        :param exception:   The exception that has occurred.
        """
        self._executor.schedule(Publisher.error, self, exception)

    def complete(self):
        """
        Completes the Publisher successfully.
        """
        self._executor.schedule(Publisher.complete, self)


########################################################################################################################

"""
Additional Thoughts
===================


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
"""
