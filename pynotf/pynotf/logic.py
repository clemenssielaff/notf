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
#   Maybe it would be a good idea to pack the <Value, Signal> pair into a single object called a `Signal`. A Signal
#   is non-copyable, but the value within it is. Signals can be ignored, blocked or handled (instead of "accepted").

########################################################################################################################

"""
Ideas
=====

Exceptions
----------
(using the new terminology from above)
With the introduction of user-written code, we inevitable open the door to user-written bugs. Therefore, during Logic 
evaluation, all Receivers and Switches are expected to be able to fail at any time.
Failure in this case means that the Receiver throws an exception instead of finishing normally. Internal errors are of
course invisible to us.
In case of a failure, the Emitter upstream will acknowledge the error and ignore it for now.
In the future, we might think of different policies, where the Emitter drops Receivers that fail once, multiple times
in a row or in total or whatever.

The alternative is that we keep track of the entire state of the application (meaning that user-defined code is no 
longer allowed to keep any state of itself) and use event-sourcing in order to facility complete roll-backs of the state
once a failure has occurred (make user-code transactional). This might be something else to think of in the future, but 
I think that for now it might be best to assume that the user-programmer is capable of producing code that does not 
break once a failure occurs.


Additional Thoughts
===================


Ownership
---------
In the Emitter-Receiver relationship, who owns who? Clearly, the Emitter must have a set (or list) of weak or
strong references to all of its Receivers, but do Receivers need to know their Emitters? And if so, do they need
to keep them alive? 

After some back and forth I now think that Receivers do not need to know about the Emitters they are connected to. 
If a Emitter goes out of scope, the Receiver will receive a completed or failure message and that's that. The 
Emitters in turn do not own their Receivers either. If a Receiver drops, the Emitter will simply remove it and 
carry on. This puts the responsibility of ownership on entities outside the Emitter-Receiver module. 


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
