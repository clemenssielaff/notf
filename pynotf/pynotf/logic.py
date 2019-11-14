"""
The Application Logic
=====================

This module contains the relevant classes to build the Application *Logic*. We use the term Logic here, because itdescribes the application behavior in deterministic [if-this-then-that terms](https://en.wikipedia.org/wiki/Logic).Whereas the Logic describes behavior in the abstract (as in: "the Logic is valid"), the actual implementation of aparticular Logic uses a terms borrowed from signal processing and the design of[electrical circuits](https://en.wikipedia.org/wiki/Electrical_network). While we go into each term into detail, letus start with the an exhaustive list of terms for reference:


Terminology
-----------

+ **Logic**

 Describes the complete behavior space of the Application. At every point in time, the Logic is expressed through the Logic *Circuit*, but while a Circuit is mutable, the Logic itself is static. Similar to how a state machine is static, while the expressed state of the machine can change.

+ **Circuit**

 A Circuit is a concrete configuration of Emitters and Receivers arranged in a directed, acyclic graph (DAG).

+ **Event**

 An Event encompasses the introduction of a new Signal into a Circuit, its propagation and the modifications of the Circuit as a result. The Event is finished, once all Receivers have finished handling the input Signal and no more Signals are being emitted within the Circuit. Unlike in Qt, there are no "Event objects".

+ **Signal**

 Object at the front of the Event handling process. At the beginning of an Event, a single Signal is emitted into the Circuit by a single Emitter but as the Signal is propagated through, it can split into different Signals.

+ **Emitter**

 Is a Circuit object that emits a Signal into the Circuit. Emitters can be sources or relays of Signals, meaning they either introduce a new Signal into the circuit from somewhere outside the Logic (see Facts in the scene module) or they can create a Signals as a response to another Signal from within the Circuit. Emitters may contain a user-defined sorting function for their connected Receivers, but most will simply use the default implementation which is based on the order of connection and connection priorities.

+ **Receiver**

 Is the counter-object to an Emitter. A Receiver receives a Signal from an Emitter and handles it in a way appropriate to its type. The handler function of a Receiver is the main injection point for user-defined functionality.

+ **Switch**

 Anything that is both an Emitter and a Receiver of Signals. Note that not all Switches generate an output Signal for each input Signal.


Directed Acyclic Circuit
------------------------

The Circuit must be a directed, acyclic graph (DAG). Cycles would be okay, if we could guarantee that Switches did nothold any state (even though infinite loops would still be possible). However, since Switches are allowed to havearbitrary, user-defined state we cannot guarantee that the callback functions in every Switch are reentrant. Basically,if the same stateful Switch is emitting multiple times in parallel, it would need to have multiple states in parallel.

Note that it is impossible to sort the Circuit statically (without executing it) since we are allowing user-definedcallbacks within the Receivers that might react differently based on the Receiver's state. Instead, we have to allowfor the possibility of user-introduced cycles at any point and handle it as gracefully as possible. To accomplish this,every Signal has encoded within it the path that it took from the original source to the current Switch. If the nextReceiver in line is already part of the Signal's path, we have detected a cycle and can interrupt the emission beforeit happens.That said, we *can* guarantee that cycles are impossible using static analysis on the Circuit. And even though a cycledetected during static analysis does not automatically mean that a cyclic dependency error will occur at runtime, itspresence is highly dubious and should be reason for a warning at least.


Ownership and Lifetimes
-----------------------

While it is obvious that Emitters must store a a list of references to their connected Receivers, whether or not Receivers need to store references to their connected Emitters is a question of design requirements. Furthermore,references come in two flavors: strong and weak. Strong reference implying ownership of the referenced object, weak ones do not. In order to avoid memory leaks we need to ensure that the graph of all strong references is a DAG.The other design consideration with strong references is that of object lifetime: ideally you want an object to stayaround exactly as long as it is needed but no longer. 

In the beginning, Receivers kept a set of strong references to their Emitters. The rationale for that decision assumed that there were certain fixed points in the Circuit (Facts, for example) that were kept alive from the outside. Receivers would spawn into the Circuit and with them a "pipeline" of Switches upstream, that would connect to one or more of these fixed points in order to generate a customized stream of data. Since the pipeline was tailor crafted for aReceiver, it made sense that the Receiver owned the pipeline and since a pipeline was a sequence of Switches, theobvious way to achieve this behavior was to have downstream Receivers own their upstream Emitters. 
<br> Let's call this the *reverse ownership* approach, because the ownership goes in the opposite direction of the data flow.

Then we introduced *Switch Operations*, which offered an easy way to construct an entire pipeline in a single Circuitelement. Instead of having n-Switches daisy-chained together, you could now have a single Switch that produces thesame result (and more efficiently so, at least in a compiled language). While this did not influence the question ofownership, it did result in Circuits with far fewer Switches. Suddenly it became feasable to burden the user with keeping track of an entire pipeline of Switches, something we considered before but ultimately deemed as too involved.

Next, we started designing the *Scene* and its *Nodes*. Nodes own a set of Receivers (called *Slots*) and Emitters. Since the life time of a Node varies from the entire duration of the session down to less than a second, we had to consider the fact that Receivers would regularly outlive Nodes with Emitters upstream. Up to then, this meant that thedownstream Receiver would keep a part of the Node alive, or even the entire Node, depending on how Nodes were designed.This was counter to the idea that Nodes should appear as a unit and also dissappear as one. The second version of the Receiver design therefore did not own *any* references to the Emitters they were connected to. If an Emitter went out of scope, the Receiver would receive a completed or failure message and that was that. The Emitters in turn did not own their Receivers either. If a Receiver droped, the Emitter would simply remove it and carry on. This moved the responsibility of ownership entirely outside the Circuit. 
<br> Let's call this the *external ownership* approach.

Eventually I realized that if Node Emitters would always finish when the Node was removed, and all Receivers were guaranteed to disconnect from a finished Emitter, the *reverse ownership* approach would still work and it would be generalizeable, since a finished (either completed or failed) Emitter will never emit again. There is no reason to keep it around. With both models feasible, we had to settle on one.

The *external ownership* approach has the advantage of being extremly light-weight. Only the bare minimum of data is stored in the circuit and since every Switch has to be owned externally (by a Node or some other mechanism), users are encouraged to keep the number of Switches reasonably small.
<br> The same could be said of the *reverse ownership* approach, but through different means. By relieving the user of the burden of keeping Switches alive, it becomes easier to construct throw-away Switches and Emitters because they live just as long as they are needed and are automatically deleted when they have finished or have lost all of their Receivers. This could lead to more Switches, but it also encourages the re-use of existing Switches since their livetime is no longer tied to some external instance. 

Overall, I think that the advantages of having the automatic lifetime management of the *reverse ownership* outweigh the space savings of the *external ownership* approach. Note that the *reverse ownership* does not have a runtime overhead, since the strong references from Receivers to Emitters do not take part in the propagation of Signals. And the space overhead is that of as many `shared_ptr`s as there are Emitters connected upstream...And I would suspect that that number is rather small.


Exception Handling
------------------

With the introduction of user-written code, we inevitable open the door to user-written bugs. Therefore, during Event handling evaluation, all Receivers have the possibility of failing at any time. Failure in this case means that theReceiver throws an exception instead of returning normally. Internal errors, that are caught and handled internally,remain of course invisible to the Logic.

In case of a failure, the exception thrown by the Receiver is caught by the Emitter upstream, that is currently in theprocess of emitting. The way that the Emitter reacts to the exception can be selected at runtime using the [delegation pattern](https://en.wikipedia.org/wiki/Delegation_pattern). The default behavior is to acknowledge theexception by logging a warning, but ultimately to ignore it, for there is no general way to handle user code errors.Other delegates may opt to drop Receivers that fail once, fail multiple times in a row or in total, etc.

---
"""
from typing import List, Optional, Any, Tuple
from abc import ABCMeta, abstractmethod
from enum import Enum, auto
from logging import error as print_error
from traceback import format_exc
from weakref import ref as weak

from .value import Value


########################################################################################################################


class Emitter:
    """
    A Emitter keeps a list of (weak references to) Receivers that are called when a new value is emitted or the
    Emitter completes, either through an error or successfully.
    Emitters emit a single Value and contain additional checks to make sure their Receivers are compatible.

    Note: This would usually be named an "Observable", while the Receiver (below) would usually be called "Observer".
    See: http://reactivex.io/documentation/observable.html
    However, these names are crap. Here is a comprehensive list why:
        - An Observable actively pushes its value downstream even though the name implies that it is purely passive.
          The Observer, respectively, doesn't do much observing at all (no polling etc.) but is instead the passive one.
        - The words Observable and Observer look very similar, especially when you just glance at the words to navigate
          the code. Furthermore, the first six characters are the same, which means that there is no way to use code-
          completion without having to type more than half of the word.
        - When you add another qualifier (like "Value"), you end up with "ObservableValue" and "ValueObserver". Or
          "ValuedObservable" and "ValueObserver" if you want to keep the "Value" part up front. Both solutions are
          inferior to "ValueEmitter" and "ValueReceiver", two words that look distinct from each other and do not
          require grammatical artistry to make sense.
        - Lastly, with "Observable" and "Observer", there is only one verb you can use to describe what they are doing:
          "to observe" / "to being observed" (because what else can they do?). This leads to incomprehensible sentences
          like: "All Observables contain weak references to each Observer that observes them, each Observer can observe
          multiple Observables while each Observables can be observed by multiple Observers".
          The same sentence using the terms Emitter and Receiver: "All Emitters contain weak references to each
          Receiver they emit to, each Receiver can connect to multiple Emitters while each Emitter can emit to multiple
          Receivers". It's not great prose either way, but at least the second version doesn't make me want to change
          my profession.

    With Emitter-Receiver, we are going for an electrical switchboard kinda-vibe. It's only fitting that the thing being
    transmitted is called a Signal and that things that ingest Signals and maybe output another one are called Switches.
    </rant>
    """

    class Signal:
        """
        Whenever a Emitter emits a new Value, it passes along a "Signal" object containing additional information
        about the source of the Signal and whether or not it can be blocked from being propagated further.
        The Signal acts as meta-data, which provides essential information for some kind of Switches and the
        Application Logic.

        The "source" of the Signal is the Emitter that emitted the Value to the Receiver. Since a Receiver is
        free to connect to any number of different Emitters, and the Value itself does not encode where it was
        generated, without the source it would be impossible for example to have an Switch that collects values from
        multiple Emitters in a list each until they complete and then emits a single concatenated Value.

        The "status" of the Signal is relevant for distributing Switches, for example one that propagates a mouse click
        to all Widgets at that location, in the draw order from top to bottom. The first Widget to accept the click
        would set the status from "unhandled" to "accepted", notifying later widgets that they should not act unless the
        programmer chooses to do so regardless. A "blocked" status signifies to the distributing Switch, that it
        should halt further propagation and return immediately, if for example we want to put a grey overlay over a
        Widget to signify that this part of the interface has been temporarily disabled without explicitly adding a
        disabled-state to the lower Widgets.
        Most Signals are "unblockable", meaning that the Receiver is unable to interfere with their propagation.
        """

        class Status(Enum):
            """
            Status of the Signal with the following state transition diagram:
                --> Unhandled --> Accepted --> Blocked
                        |                        ^
                        +------------------------+
                --> Unblockable
            """
            UNBLOCKABLE = 0
            UNHANDLED = auto()
            ACCEPTED = auto()
            BLOCKED = auto()

        def __init__(self, emitter: 'Emitter', is_blockable: bool = False):
            self._emitter: Emitter = emitter
            self._status: Emitter.Signal.Status = self.Status.UNHANDLED if is_blockable else self.Status.UNBLOCKABLE

        @property
        def source(self) -> int:
            """
            Numeric ID of the Emitter that emitted the value.
            """
            return id(self._emitter)

        def is_blockable(self) -> bool:
            """
            Checks if this Signal could be blocked. Note that this also returns true if the Signal has already been
            blocked and blocking it again would have no effect.
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

    def __init__(self, schema: Value.Schema = Value.Schema()):
        """
        Constructor.
        :param schema: Schema of the Value emitted by this Emitter, defaults to the empty Schema.
        """

        self._receivers: List[weak] = []
        """
        Receivers are unique but still stored in a list so we can be certain of their call order, which proves
        convenient for testing.
        """

        self._is_completed: bool = False
        """
        Once a Emitter is completed, it will no longer emit any values.
        """

        self._output_schema: Value.Schema = schema
        """
        Is constant.
        """

    @property
    def output_schema(self) -> Value.Schema:
        """
        Schema of the emitted value. Can be the empty Schema if this Emitter does not emit a meaningful value.
        """
        return self._output_schema

    def is_completed(self) -> bool:
        """
        Returns true iff the Emitter has been completed, either through an error or normally.
        """
        return self._is_completed

    def emit(self, value: Any = Value()):
        """
        Push the given value to all active Receivers.
        :param value: Value to emit, can be empty if this Emitter does not emit a meaningful value.
        :raise TypeError: If the Value's Schema doesn't match.
        :raise RuntimeError: If the Emitter has already completed (either normally or though an error).
        """
        # First, the Emitter needs to check if it is already completed. No point doing anything, if the Emitter is
        # unable to emit anything. This should never happen, so raise an exception if it does.
        if self._is_completed:
            raise RuntimeError("Cannot emit from a completed Emitter")

        # Check the given value to see if the Emitter is allowed to emit it. If it is not a Value, try to
        # build one out of it. If that doesn't work, or the Schema of the Value does not match that of the
        # Emitter, raise an exception.
        if not isinstance(value, Value):
            try:
                value = Value(value)
            except ValueError:
                raise TypeError("Emitter can only emit values that are implicitly convertible to a Value")
        if value.schema != self.output_schema:
            raise TypeError("Emitter cannot emit a value with the wrong Schema")

        # Emit from the local list of strong references that we know will stay alive. The member field may change
        # during the iteration because some Receiver downstream might add to the list of Receivers or even disconnect
        # other Receivers, but those changes will not affect the current emitting process.
        self._emit(self._collect_receivers(), value)

    def _error(self, exception: Exception):
        """
        Failure method, completes the Emitter.
        This method would be protected in C++. It is not private, but should not be part of the public interface.
        :param exception:   The exception that has occurred.
        """
        print_error(format_exc())

        signal: Emitter.Signal = Emitter.Signal(self, is_blockable=False)
        for receiver in self._collect_receivers():
            try:
                receiver.on_error(signal, exception)
            except Exception as ex:
                print_error(format_exc())

        self._receivers.clear()
        self._is_completed = True

    def _complete(self):
        """
        Completes the Emitter successfully.
        This method would be protected in C++. It is not private, but should not be part of the public interface.
        """
        signal: Emitter.Signal = Emitter.Signal(self, is_blockable=False)
        for receiver in self._collect_receivers():
            try:
                receiver.on_complete(signal)
            except Exception as ex:
                print_error(format_exc())

        self._receivers.clear()
        self._is_completed = True

    def _collect_receivers(self) -> List['Receiver']:
        """
        Remove all expired Receivers and return a list of strong references to the valid ones.
        Note that the order of Receivers does not change.
        """
        receivers = []
        valid_index = 0
        for current_index in range(len(self._receivers)):
            receiver = self._receivers[current_index]()
            if receiver is not None:
                receivers.append(receiver)
                self._receivers[valid_index] = self._receivers[current_index]  # would be a move or swap in C++
                valid_index += 1
        self._receivers = self._receivers[:valid_index]
        return receivers

    def _emit(self, receivers: List['Receiver'], value: Value):
        """
        This method can be overwritten in subclasses to modify the emitting process.
        At the point where this method is called, we have established that `value` can be emitted by this Emitter,
        meaning its Schema matches the output Schema of this Emitter and the Emitter has not been completed. All
        expired Receivers have been removed from the list of Receivers and the `receivers` argument is a list of strong
        references matching the `self._receivers` weak list.
        The default implementation of this method simply emits an unblockable Signal to every Receiver in order.
        Subclasses can do further sorting and if they choose to, can re-apply that ordering to the `_receivers`
        member field in the hope to speed up sorting the same list in the future using the `_sort_receivers` method.
        :param receivers: List of Receivers to emit to.
        :param value: Value to emit.
        """
        signal = Emitter.Signal(self, is_blockable=False)
        for receiver in receivers:
            try:
                receiver.on_next(signal, value)
            except Exception as exception:
                self._handle_exception(receiver, exception)

    def _sort_receivers(self, order: List[int]):
        """
        Changes the order of the Receivers of this Emitter without giving write access to the `_receivers` field.
        Example: Given Receivers [a, b, c], then calling `self._sort_receivers([2, 0, 1])` will change the order
        to [c, a, b]. Invalid orders will raise an exception.
        :param order: New order of the receivers.
        :raise RuntimeError: If the given order is invalid.
        """
        if sorted(order) != list(range(len(self._receivers))):
            raise RuntimeError(f"Invalid order: {order} for a Emitter with {len(self._receivers)} Receivers")

        self._receivers = [self._receivers[index] for index in order]

    def _connect(self, receiver: 'Receiver'):
        """
        Adds a new Receiver to receive emitted values.
        :param receiver: New Receiver.
        :raise TypeError: If the Receiver's input Schema does not match.
        """
        if receiver.input_schema != self.output_schema:
            raise TypeError("Cannot connect to an Emitter with a different Schema")

        if self._is_completed:
            receiver.on_complete(Emitter.Signal(self))

        else:
            weak_receiver = weak(receiver)
            if weak_receiver not in self._receivers:
                self._receivers.append(weak_receiver)

    def _disconnect(self, receiver: 'Receiver'):
        """
        Removes the given Receiver if it is connected. If not, the call is ignored.
        :param receiver: Receiver to disconnect.
        """
        weak_receiver = weak(receiver)
        if weak_receiver in self._receivers:
            self._receivers.remove(weak_receiver)

    def _handle_exception(self, receiver: 'Receiver', exception: Exception):
        """
        This is a virtual method that Emitters can override in order to handle exceptions that occurred in their
        Receivers `on_next` method.
        The default implementation simply reports the exception but ultimately ignores it.
        :param receiver: Receiver that raised the exception.
        :param exception: Exception raised by receiver.
        """
        print_error("Receiver {} failed during Logic evaluation.\nException caught by Emitter {}:\n{}".format(
            id(receiver), id(self), exception))


########################################################################################################################

class Receiver(metaclass=ABCMeta):
    """
    Receivers do not keep track of the Emitters they connect to.
    An Receiver can be connected to multiple Emitters.
    """

    def __init__(self, schema: Value.Schema = Value.Schema()):
        """
        Constructor.
        :param schema:  Schema defining the Value expected by this Emitter.
                        Can be empty if this Receiver does not expect a meaningful value.
        """
        self._input_schema: Value.Schema = schema
        """
        Is constant.
        """

    @property
    def input_schema(self):
        """
        The expected value type.
        """
        return self._input_schema

    @abstractmethod
    def on_next(self, signal: Emitter.Signal, value: Optional[Value]):
        """
        Abstract method called by any upstream Emitter.
        :param signal   The Signal associated with this call.
        :param value    Emitted value, can be None.
        """
        pass

    def on_error(self, signal: Emitter.Signal, exception: Exception):
        """
        Default implementation of the "error" method: does nothing.
        :param signal   The Signal associated with this call.
        :param exception:   The exception that has occurred.
        """
        pass

    def on_complete(self, signal: Emitter.Signal):
        """
        Default implementation of the "complete" operation, does nothing.
        :param signal   The Signal associated with this call.
        """
        pass

    def connect_to(self, emitter: Emitter):
        """
        Connect to the given Emitter.
        :param emitter:   Emitter to connect to. If this Receiver is already connected, this does nothing.
        :raise TypeError:   If the Emitters's input Schema does not match.
        """
        emitter._connect(self)

    def disconnect_from(self, emitter: Emitter):
        """
        Disconnects from the given Emitter.
        :param emitter:   Emitter to disconnect from. If this Receiver is not connected, this does nothing.
        """
        emitter._disconnect(self)


########################################################################################################################

class Switch(Receiver, Emitter):
    """
    A Switch is Receiver/Emitter that applies a sequence of Operations to an input value before emitting it
    (see https://en.wikipedia.org/wiki/Switch).
    If any Operation throws an exception, the Switch will fail as a whole. Operations are not able to complete the
    Switch, other than through failure.
    Not every input is guaranteed to produce an output as Operations are free to store or ignore input values.
    If an Operation returns a value, it is passed on to the next Operation and if the Operation was the last one in the
    sequence, its return value is emitted by the Switch.
    If any Operation does not return a value (returns None), the following Operations are not called and the Switch
    does not emit anything.
    """

    class Operation(metaclass=ABCMeta):
        """
        Operations are Functors that can be part of a Switch.
        """

        @property
        @abstractmethod
        def input_schema(self) -> Value.Schema:
            """
            The Schema of the input value of the Operation.
            """
            pass

        @property
        @abstractmethod
        def output_schema(self) -> Value.Schema:
            """
            The Schema of the output value of the Operation (if there is one).
            """
            pass

        @abstractmethod
        def _perform(self, value: Value) -> Optional[Value]:
            """
            Operation implementation.
            :param value: Input value, conforms to the Schema specified by the `input_schema` property.
            :return: Either a new output value (conforming to the Schema specified by the `output_schema` property)
                or None.
            """
            pass

        def __call__(self, value: Value) -> Optional[Value]:
            """
            Perform the Operation on a given value.
            :param value: Input value, must conform to the Schema specified by the `input_schema` property.
            :return: Either a new output value (conforming to the Schema specified by the `output_schema` property)
                or None.
            :raise TypeError: If the input Value does not conform to this Operation's input Schema.
            """
            if not isinstance(value, Value) or value.schema != self.input_schema:
                raise TypeError("Value does not conform to the Operation's input Schema")
            result: Optional[Value] = self._perform(value)
            if result is not None:
                assert result.schema == self.output_schema
            return result

    class NoOp(Operation):
        """
        An Operation that passes the given Value onwards.
        Used to create valid but empty Operators.
        """

        def __init__(self, schema: Value.Schema):
            """
            Constructor.
            :param schema: Schema of the Value to pass through.
            """
            self._schema: Value.Schema = schema

        @property
        def input_schema(self) -> Value.Schema:
            return self._schema

        @property
        def output_schema(self) -> Value.Schema:
            return self._schema

        def _perform(self, value: Value) -> Value:
            """
            :param value: Input value.
            :return: Unchanged input value.
            """
            return value

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

        Receiver.__init__(self, self._operations[0].input_schema)
        Emitter.__init__(self, self._operations[-1].output_schema)

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

    def on_next(self, signal: Emitter.Signal, value: Value):
        """
        Performs the sequence of Operations on the given Value and emits the result if one is produced.
        Exceptions thrown by a Operation will cause the Switch to fail.

        :param signal   The Signal associated with this call.
        :param value    Input Value conforming to the Switch's input Schema.
        """
        try:
            result = self._operate_on(value)
        except Exception as exception:
            self._error(exception)
        else:
            if result is not None:
                self.emit(result)


# TODO: better Signals
#   * Maybe it would be a good idea to pack the <Value, Signal> pair into a single object called a `Signal`. A Signal
#   is non-copyable, but the value within it is. Signals can be ignored, blocked or handled (instead of "accepted").
#   * Also, Signal emitter field should not be a single emitter but any ordered set of Emitters. Ordered, so you can
#   recreate the path of the Signal to find out the latest emitter and for debugging, and a set to quickly check for
#   loops at run-time


########################################################################################################################

"""


Additional Thoughts
===================







Propagated Data & Signal
--------------------------
At some point I decided that the only thing to be handed out by Emitters should be Values to allow complete
introspection of the system and to not have any types that are only accessible from C++, for example. This works fine,
as long as the only thing that got passed around is pure, immutable data. Immutable being the weight bearing word here.

Then it came time to think about how basic basic events like a mouse click are propagated in the system. In previous 
versions, I had taken the Qt approach of using dedicated Event objects that were given to each applicable Widget in 
draw stack-order, with higher Widgets receiving the event first and lower Widgets later. Every Widget could "accept" the
Event, which would set a flag on the Event object itself notifying later Widgets that the Event was already handled and 
that they don't have to act if they don't want to. 
This of course requires the given data to be mutable, which isn't possible with Values.

Here a (hopefully) comprehensive list of solutions to the problem:
    1.`Receiver.on_next` could return a bool to let the Scheduler know if it should continue propagating the event.
    2.`Receiver.on_next` could return the (optionally modified) Value to propagate further.
    3. Add a mutable extra-argument to `Receiver.on_next`, so the Value itself remains immutable.
    4. Have a global / ApplicationLogic / Graph state for the currently handled Event that can be modified.
    5. Allow non-Value propagation and simply propagate classic Event objects.
    6. Make Event handling a separate "thing" from the Application Logic, effectively sidestepping the problem.

Discussion:
    1. Easy, but does not allow handled Events to be propagated. If you want a foreground button and a nice background 
       effect whenever you click on the button, the button must know it and return "False" even though the Event was 
       handled by it, just so that the background still receives the Event. If another button moves in between the first
       and the background, it will not know that the Event was handled. A nightmare. Declined. 
    2. This is actually really interesting... You could, for example, strip or add modifier keys to a mouse click on a
       part of a Widget that lies on top of another one, so the lower Widget would then receive the modified Event 
       instead. This is truly general and since Values are shared pointers anyway, should also be really cheap to return
       unmodified values. And it's not like you can return just anything, a Value can only be modified within its Schema
       so the "type" of Value will stay the same.
       However, it is really easy to mess around with the Event handling to a point where pinpointing errors could 
       become a hassle. There is no mechanism protecting handled Events to become unhandled again since Values do not
       encode any internal logic. Additionally, this would have to be the general case, where each Receiver would need
       to return the Value even if no Event handling is happening. And since Receivers (in the general case) are 
       called in essentially random order, passing on a possibly modified value from one to the next is a potential
       debugging nightmare.
    3. This would work. Instead of the Emitter that emits the value, we can pass a structure that has 
       additional mutable and immutable information, like the Emitter that emitted the value, but also whether or 
       not the event was accepted. It would be nice to generalize this approach though, so we don't have to encode
       event acceptance into the Scheduler. Previously, this was part of the Event class, which is nice because if you
       want a different kind of behavior (maybe an event that can actually be blocked), you just add it to the specific
       Event type. But maybe handled or not is general enough?
    4. Possible, but each with drawbacks. A global state is easily accessible but does not work with multiple instances.
       ApplicationLogic and Graph both require the Receiver to keep a reference to them, even though they are hardly
       ever used.
    5. I wanted to avoid that in the general case, because it would be nice to a have a Logic DAG consisting of only 
       data flow, no additional logic encapsulated within the data, only in the Operators. Then again, allowing other 
       types than Values to be passed isn't really a "new" concept, it is more a generalization of an existing one. 
       It won't even change the behavior much, because Schemas act as quasi-typing for Values and so you were never 
       allowed to just connect any two arbitrary Emitter/Receiver pairs anyway. 
    6. Intriguing, but this would add yet another concept whereas I am trying to reduce the number of different concepts
       that make up the system.

Decision:
    This was a tough one. I stared out liking option 3 a lot, then number 2 until I realized how brittle that would be,
    then switched to number 5 as the "sane" default option that only required me to give up the impossible idea of
    building the Application Logic using only Values and Operators. However, I have since gotten around to favor number 
    3 again, hopefully for good this time.
    Here is the problem with number 5: It is probably the best solution for this special case: handling a well-defined
    (in this case a mouse-click) problem by introducing a new, well defined type is a straight-forward approach for 
    every object oriented programmer. But it requires Receivers and Emitters that are both aware of the type and 
    know how to work with it. This if fine in this example alone, but the more I thought about it, the more I realized
    that we would need a truly general solution.
    
    To illustrate the problem, let's take the simple example from above: a simple mouse click should be propagated to 
    all Widgets that are interested in its effects. To make things interesting, the click can either be a single mouse 
    click or the second click in a double-click event! Here is the Application Logic that I originally envisioned:

                                                 +---------------+
                                                 |    Filter     |
                                                 | Single Click  |
                                                 | ------------- |    +-------------- +
                                              +---> Click Value  |    |  Distributor  |
        +-------------+    +---------------+  |  |   & count     |    | Single Click  |
        |    Fact     |    |   Switch    |  |  | ------------- |    | ------------- |
        | Mouse Click |    | Click Counter |  |  |   Click Value +-----> Click Value  |           +------------+
        | ----------- |    | ------------- |  |  +---------------+    | ------------- |           |            |
        | Click Value +-----> Click Value  |  |                       |   Click Event +------------> Widget 1  |
        +-------------+    | ------------- |  |                       +---------------+        |  |            |
                           |   Click Value +--+                                                |  +------------+
                           |    & count    |  |  +---------------+                             |
                           +---------------+  |  |    Filter     |                             |  +------------+
                                              |  | Double Click  |                             +--->           |
                                              |  | ------------- |    +---------------------+     |  Widget 2  |
                                              +---> Click Value  |    |     Distributor     |  +--->           |
                                                 |   & count     |    |     Double Click    |  |  +------------+
                                                 | ------------- |    | ------------------- |  |
                                                 |   Click Value +-----> Click Value        |  |  +------------+
                                                 +---------------+    | ------------------- |  |  |            |
                                                                      |  Double Click Event +------> Widget 3  |
                                                                      +---------------------+     |            |
                                                                                                  +------------+

    At the beginning, there is the simple Fact that a mouse click has occurred, alongside some additional information
    like the position of the cursor and the button that was clicked. Whenever a click happens, it flows immediately into
    the "Click Counter" Switch that counts the number of clicks within a given time in the past and adds that number
    to the Click Value. Downstream of the counter, we have two filters that check the click count number, strip it from
    the output Value and fire only if it is a single click or a second click respectively. Both filters then flow into
    a dedicated Distributor each, which turns the Click Value into a (Double) Click Event and propagates it to the 
    interested Widgets according to their draw order. Widget 1 is only interested in single click events, Widget 3 only 
    in double-clicks and Widget 2 in both.
    Looks good so far. We could even easily add a triple-click event and wouldn't have to change the existing network
    at all. What this story misses however, is that we don't get "click" messages, we only get "mouse up" messages. This
    only translates into a "click" if the mouse wasn't moved while it was down ... or is it a "click" regardless? What
    if the mouse was held down for a significant amount of time? Maybe to select a span in a playing audio file? Is this
    a click as well?
    The problem is that there are many different ways to interpret a system event like a "mouse up" and not all of them
    are as cleanly cut as "is this the second click in the last x milliseconds". In the example, there was a dedicated 
    Switch to count clicks. This seems fairly general. But what about an Switch to decide whether this was a click, 
    a gesture, etc? Suddenly, this Switch has a lot of logic inside, written as code. This is not the general solution 
    that we want. 
    What we need here is an "Ordered X-OR" Switch, or however you want to call it. It is a way to make sure that of n 
    different Operators, at most one will actually generate a Value -- or in terms of the example: a mouse up event is 
    either a click, the end of a drag, the end of a gesture or something else. But it should not be more than one of 
    these things. The Switch keeps its Receivers ordered in a list and the first Receiver to report that it has
    successfully accepted the Value, is the last one to receive it... sounds familiar. So maybe we should add another
    type, something like "Pre-Event", since at this point, the "mouse up" isn't yet what we called an Event earlier?
    Of course, we'd need a dedicated Switch to spit out these "Pre-Event" values and all of the Receivers need to
    accept them.
    ... At this point I realized that option number 5 was a dead end as well.
    Which finally brings us to:

Signal:
    Early on, when designing a reactive module, I decided that Emitters should always pass their `this` pointer 
    alongside the emitted value, for identification purposes only. This way the Receiver was free to sort values
    from different Emitters into different Buckets for example, and concatenate all of them when each Emitter had
    completed. I don't really know how else to implement this feature and it seems like a straightforward, easy and
    cheap thing to do.
    With the need for some kind of feedback from the Receiver back to the Emitter, I think I have found yet another
    use-case for meta data to accompany any emitted value that will be ignored by some/most, but offers indispensable
    features to others. And instead of adding yet another argument to the `Receiver.on_next` function, it seems 
    reasonable to replace the const pointer back to the Emitter with a mutable reference to what we shall call Signal. 
    So far, it consists of only two things:
        1. Some kind of ID that uniquely identifies the Emitter from all other possible Emitters that the Receiver
           might connect to. This ID is constant. It can be the memory address of the Emitter or some other integer.
        2. An enum that describes the Status of the Signal, encapsulated in an object to ensure that it follows the 
           state transition diagram outlined below:
            
            --> Unhandled --> Handled --> Blocked 
                    |                        ^
                    +------------------------+
                    
            --> Unblockable

           The Status always starts out as either "Unhandled" or "Unblockable", with "Unblockable" being the default.
           An unblockable Signal cannot be stopped and calls to `set_handled()` or `block()` are ignored. If the Status
           starts out as "Unhandled", then each Receiver is free to mark the Status as handled or blocked. A handled
           Signal is usually propagated further (depending on how the Emitter chooses to interpret what "handled" 
           means in its specific use-case), whereas the Receiver that marks its Signal as being blocked, is the last
           one to receive it.  
     

Logic Modifications
-------------------
Unlike static DAGs, we allow Operators and other callbacks to modify the Logic "in flight", while an event is processed. 
This can lead to several problems.
Problem 1:
    
        +-> B 
        |
    A --+
        |
        +-> C
    
    Lets assume that B and C are Receivers that are connected to updates from Emitter A. B reacts by removing C and
    adding the string "B" to a log file, while C reacts by removing B and adding "C" to a log file. Depending on which 
    Receiver receives the update first, the log file with either say "B" _or_ "C". Since not all Emitters adhere to 
    an order in their Receivers, the result of this setup is essentially random.

Problem 2:
    
    A +--> B
      | 
      +..> C

    B is a Slot of a canvas-like Widget. Whenever the user clicks into the Widget, it will create a new child Widget C, 
    which an also be clicked on. Whenever the user clicks on C, it disappears. The problem arises when B receives an 
    update from A, creates C and immediately connects C to A. Let's assume that this does not cause a problem with A's
    list of Receivers changing its size mid-iteration. It is fair to assume that C will be updated by A after B has 
    been updated. Therefore, after B has been updated, the *same* click is immediately forwarded to C, which causes it 
    to disappear. In effect, C will never show up.

There are multiple ways these problems could be addressed.
Solution 1:
    Have a Scheduler determine the order of all updates beforehand. During that step, all expired Receivers are 
    removed and a backup strong reference for all live Receivers are stored in the Scheduler to ensure that they
    survive (which mitigates Problem 1). Additionally, when new Receivers are added to any Emitter during the update
    process, they will not be part of the Scheduler and are simply not called.

Solution 2:
    A two-phase update. In phase 1, all Emitters weed out expired Receivers and keep an internal copy of strong
    references to all live ones. In phase 2, this copy is then iterated and any changes to the original list of
    Receivers do not affect the update process.

Solution 3:
    Do not allow the direct addition or removal of connections and instead record them in a buffer. This buffer is 
    then executed at the end of the update process, cleanly separating the old and the new DAG state.

I have opted for solution 2, which allows us to keep the scheduling of Receivers contained within the individual
Emitter directly upstream. In order to make this work, emitting becomes a bit more complicated but not by a lot.


Ordered Emissions
-----------------
(Side-note: At the time of this writing, these specs did not guarantee any order during Signal emission, meaning that 
emission order was essentially random. "Essentially" because it did not have to be random, but there was no rule 
prohibiting it either.)

I already discussed the problem with the removal on nodes mid-event. To reiterate:
Given one Emitter `E1` and two Slots `S1` and `S2` of different Nodes `N1` and `N2` arranged in the following circuit:

```
         +---> S1(N1) -> H1
    E1 --+
         +---> S2(N2) -> H2
```

Both `S1` and `S2` are connected to a Handler, let's call them `H1` and `H2`. Both Handlers are allowed to execute
arbitrary user-code. The problem arises when `H1` removes `N2` and `H2` removes `H1`. Since the order in which `S1` and 
`S2` receive the Signal from `E1` is essentially random, the event might either remove `N1` *or* `N2`, but never both
and we don't know which it will be. This is confusing, hard to debug and certainly never what the user intended. 

The solution to this particular problem was to delay the actual removal of Nodes until the very end of the event, when
the Signal had time to fully propagate through the circuit. This works and I thought it was enough.

However, it turns out that this problem is further reaching than I initially assumed. It is trivial to modify the 
example above to achieve the same, uncertain result without involving Nodes.
Given one Emitter `E1`, two Switches `A1` (that always adds two to the input number) and `A2` (that always adds three)
and a special Switch `S1` that is connected to both `N1` and `N2`. The circuit is laid out as follows:

```
          +---> A1 (adds 2) ---+
          |                     \
    E1 ---+                      S1 
          |                     /
          +---> A2 (adds 3) ---+
```

The particular behavior of `S2` is that it ingests two numbers `x` and `y` and after receiving the second one, produces 
`z` with `z = x - y`. It then resets and waits for the next pair. 
The problem is that if `E1` propagates the number one first to either `N1` or `N2` (because, as of now the order is 
essentially random), `x` will either be 3 or 4, while `y` will be the other one. That results in either `z = 3 - 4 = -1`
or `z = 4 - 3 = 1`. Which one we get is as random as the order of propagation from `E1`.

Unlike with the Node removal above, there is no way to delay here until the event is handled because it is the event 
handling itself, that is broken. 
... So where does that leave us?

I think the only way to address the bigger problem is to get rid of the randomness. We already have a deterministic 
order in some of the Emitters (the ones that sort by Node z-value) and that should be the default. Note that I did not
say that the order must be fixed, the sorting Emitters for example will re-order their Receivers as the Scene Graph is
modified and there might be other use-cases where dynamic sorting is appropriate. What I do say however is that the
order must be fixed, inspectable and modifiable by the user. All of the problems, including the Node removal mid-event
would be solved, if we place the responsibility of the order in which Signals are emitted on the user.
Question is, how do we do that?

The only way to relate independent Receivers to each other is through a common ranking system. Fortunately there is one:
natural numbers, which basically means priorities. Whenever a Receiver connects to an Emitter, it has the opportunity
to pass an optional priority number, with higher priority Receiver receiving Signals before lower priority Receiver. The 
default priority is zero. Receivers with equal priority receive their Signals in the order in which they subscribed.
This approach allows the user to ignore priorities in most cases (since in most cases, you should not care) and in
the cases outlined above, the user has the ability to manually determine an order. The user is also free to define a
total order for each Receiver, should that ever become an issue.
New Receivers are ordered behind existing ones, not before them. You could make a point that prepending new ones to the
list of Receivers would make sure that new Receivers always get the Signal and are not blocked, but I think that 
argument is not very strong. A good argument for the other side is that appending new Receivers to the end means that
the existing Logic is undisturbed, meaning there is as little (maybe no) change in how the circuit operates as a whole.
That should be the default behavior. 

Of course, this approach does not protect the user from the error cases outlined above - but it will make them 
deterministic. I think it is a good trade-off though. By allowing user-injected, stateful code in our system, we not
only pass power to the user but also responsibility (cue obligatory Spiderman joke). And it makes for a good use-case 
for the notf Designer that allows the user to visualize the Logic circuit and Signal processing in detail...

Addendum: 
There is still the matter of Emitters keeping their connected Receivers alive during emission. This is actually only a 
side effect of the original intend: to keep the list of receivers *fixed* during emission, meaning no  removals and no 
additions. Now that we allow immediate removal and disconnection though, this might no longer feasible. If `A` deletes 
`B` then `B` should no longer receive Signals. But what if `A` *adds* `B`? Should `B` then receive the Signal 
immediately? I would say no.

Actually either way, the current behaviour of keeping the list of receivers fixed during emission is not good enough. It
might protect a single emitter, but what about connected emitters downstream? If we have 3 Switches `A`, `B` and `C` 
with `A` connected to `B` and `C`. `A` emits to `B` first, then to `C`. `B` connects a new Switch `D` downstream of `C`.
Since `C` is not currently emitting, this causes `C` to modify its list of receivers and emit the Signal to `D` right 
away - within the same loop. If however `C` connected a new receiver `D'`to `B`, then `D'` would not receive anything 
until the next time that `A` emitted. 
This is correct and will work, but it could still be confusing. The alternative would be to say that all receiver lists 
stay fixed. But then we couldn't remove mid-event either, which means that you cannot remove Nodes mid - event.

I guess ultimately we should choose the easier option first. We need ordered evaluation and we need a rule whereby 
Emitters, once emitting, can no longer change their Signal or list of Receivers to avoid a receiver type "dos"ing an 
upstream Emitter.
What about an Emitter A that emits to B and C, but B removes C? I guess the Emitter list is fixed, but should be 
non-owning.


Dead Ends
========= ==============================================================================================================
*WARNING!* 
The chapters below are only kept as reference of what doesn't make sense, so that I can look them up once I re-discover 
the "brilliant" idea underlying each one. Each chapter has a closing paragraph on why it actually doesn't work.


Scheduling (Dead End)
---------------------
When the application executor schedules a new value to be emitted, it could determine the effect of the change by
traversing the dependency DAG prior to the actual change. Take the following DAG topology:

        +-> B ->+
        |       |  
    A --+       +-> D
        |       |
        +-> C ->+ 

Whenever A changes, it will update B, which in turn will update D. A will then continue to update C, which will in turn
update D a second time. This would have been avoided if we scheduled the downstream propagation and would have updated B
and C before D.
While this particular example could be fixed by simply doing a breadth-first traversal instead of a depth-first one, 
this solution does not work in the general case, as can be seen with the following example:

        +-> B --> E ->+
        |             |  
    A --+             +-> D
        |             |
        +-> C ------->+ 

In order to guarantee the minimum amount of work, we will need a scheduler.
However, this is an optimization only - even the worst possible execution order will still produce the correct result
eventually.

Except...
None of this makes sense in the context of streams. Every single value is as important as the next one and if D receives
two downstream values from A, so be it. The order of values may change (which is it's very own topic discussed in the
application logic), but that's why we pass the direct upstream Emitter alongside the value: to allow the Receiver to
apply an order per Emitter on the absolute order in which it receives values.

"""
