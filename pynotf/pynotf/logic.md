# The Application Logic

This module contains the relevant classes to build the Application *Logic*. We use the term Logic here, because it describes the application behavior in deterministic [if-this-then-that terms](https://en.wikipedia.org/wiki/Logic). Whereas the Logic describes behavior in the abstract (as in: "the Logic is valid"), the actual implementation of a particular Logic uses a terms borrowed from signal processing and the design of [electrical circuits](https://en.wikipedia.org/wiki/Electrical_network). While we go into each term into detail, let us start with the an exhaustive list of terms for reference:


## Terminology

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

An experienced application programmer might find some of these patterns familiar. In fact, what we call an _Emitter_ and _Receiver_ could just as well be called an _Observable_ and _Observer_ to follow the _ReactiveX_ naming scheme (see [reactivex.io](http://reactivex.io/documentation/observable.html))).
<br> However, these names are not very programmer-friendly so we chose to go with our own naming. Here is a comprehensive list why:
* An Observable actively pushes its value downstream even though the name implies that it is purely passive. The Observer, respectively, doesn't do much observing at all (no polling etc.) but is instead the passive one.
* The words Observable and Observer look very similar, especially when you just glance at the words to navigate the code. Furthermore, the first six characters are the same, which means that there is no way to use code-completion without having to type more than half of the word.
* When you add another qualifier (like "Value"), you end up with "ObservableValue" and "ValueObserver". Or "ValuedObservable" and "ValueObserver" if you want to keep the "Value" part up front. Both solutions are inferior to "ValueEmitter" and "ValueReceiver", two words that look distinct from each other and do not require grammatical artistry to make sense.
* Lastly, with "Observable" and "Observer", there is only one verb you can use to describe what they are doing: "to observe" / "to being observed" (because what else can they do?). This leads to incomprehensible sentences like: "All Observables contain weak references to each Observer that observes them, each Observer can observe multiple Observables while each Observables can be observed by multiple Observers". The same sentence using the terms Emitter and Receiver: "All Emitters contain weak references to each Receiver they emit to, each Receiver can connect to multiple Emitters while each Emitter can emit to multiple Receivers". It's not great prose either way, but at least the second version doesn't make me want to change my profession.


## Directed Acyclic Circuit

The Circuit must be a directed, acyclic graph (DAG). Cycles would be okay, if we could guarantee that Switches did not hold any state (even though infinite loops would still be possible). However, since Switches are allowed to have arbitrary, user-defined state we cannot guarantee that the callback functions in every Switch are reentrant. Basically,if the same stateful Switch is emitting multiple times in parallel, it would need to have multiple states in parallel.

Note that it is impossible to sort the Circuit statically (without executing it) since we are allowing user-defined callbacks within the Receivers that might react differently based on the Receiver's state. Instead, we have to allow for the possibility of user-introduced cycles at any point and handle it as gracefully as possible. To accomplish this,every Signal has encoded within it the path that it took from the original source to the current Switch. If the nextReceiver in line is already part of the Signal's path, we have detected a cycle and can interrupt the emission before it happens.That said, we *can* guarantee that cycles are impossible using static analysis on the Circuit. And even though a cycle detected during static analysis does not automatically mean that a cyclic dependency error will occur at runtime, its presence is highly dubious and should be reason for a warning at least.


## Ownership and Lifetimes

While it is obvious that Emitters must store a a list of references to their connected Receivers, whether or not Receivers need to store references to their connected Emitters is a question of design requirements. Furthermore,references come in two flavors: strong and weak. Strong reference implying ownership of the referenced object, weak ones do not. In order to avoid memory leaks we need to ensure that the graph of all strong references is a DAG.The other design consideration with strong references is that of object lifetime: ideally you want an object to stay around exactly as long as it is needed but no longer. 

In the beginning, Receivers kept a set of strong references to their Emitters. The rationale for that decision assumed that there were certain fixed points in the Circuit (Facts, for example) that were kept alive from the outside. Receivers would spawn into the Circuit and with them a "pipeline" of Switches upstream, that would connect to one or more of these fixed points in order to generate a customized stream of data. Since the pipeline was tailor crafted for aReceiver, it made sense that the Receiver owned the pipeline and since a pipeline was a sequence of Switches, the obvious way to achieve this behavior was to have downstream Receivers own their upstream Emitters. 
<br> Let's call this the *reverse ownership* approach, because the ownership goes in the opposite direction of the data flow.

Then we introduced *Switch Operations*, which offered an easy way to construct an entire pipeline in a single Circuit element. Instead of having n-Switches daisy-chained together, you could now have a single Switch that produces the same result (and more efficiently so, at least in a compiled language). While this did not influence the question of ownership, it did result in Circuits with far fewer Switches. Suddenly it became feasible to burden the user with keeping track of an entire pipeline of Switches, something we considered before but ultimately deemed as too involved.

Next, we started designing the *Scene* and its *Nodes*. Nodes own a set of Receivers (called *Slots*) and Emitters. Since the life time of a Node varies from the entire duration of the session down to less than a second, we had to consider the fact that Receivers would regularly outlive Nodes with Emitters upstream. Up to then, this meant that the downstream Receiver would keep a part of the Node alive, or even the entire Node, depending on how Nodes were designed.This was counter to the idea that Nodes should appear as a unit and also disappear as one. The second version of the Receiver design therefore did not own *any* references to the Emitters they were connected to. If an Emitter went out of scope, the Receiver would receive a completed or failure message and that was that. The Emitters in turn did not own their Receivers either. If a Receiver dropped, the Emitter would simply remove it and carry on. This moved the responsibility of ownership entirely outside the Circuit. 
<br> Let's call this the *external ownership* approach.

Eventually I realized that if Node Emitters would always finish when the Node was removed, and all Receivers were guaranteed to disconnect from a finished Emitter, the *reverse ownership* approach would still work and it would be generalizable, since a finished (either completed or failed) Emitter will never emit again. There is no reason to keep it around. With both models feasible, we had to settle on one.

The *external ownership* approach has the advantage of being extremely light-weight. Only the bare minimum of data is stored in the circuit and since every Switch has to be owned externally (by a Node or some other mechanism), users are encouraged to keep the number of Switches reasonably small.
<br> The same could be said of the *reverse ownership* approach, but through different means. By relieving the user of the burden of keeping Switches alive, it becomes easier to construct throw-away Switches and Emitters because they live just as long as they are needed and are automatically deleted when they have finished or have lost all of their Receivers. This could lead to more Switches, but it also encourages the re-use of existing Switches since their lifetime is no longer tied to some external instance. 

Overall, I think that the advantages of having the automatic lifetime management of the *reverse ownership* outweigh the space savings of the *external ownership* approach. Note that the *reverse ownership* does not have a runtime overhead, since the strong references from Receivers to Emitters do not take part in the propagation of Signals. And the space overhead is that of as many `shared_ptr`s as there are Emitters connected upstream...And I would suspect that that number is rather small.


## What is in a Signal

Early on, we decided that the only thing to emitted by Emitters should be Values, instead of built-in or user-defined types. This would allow complete introspection of the system in any environment, and would allow the seamless interaction of Circuit elements defined in compiled languages like C++ and interpreted ones like Python. This approach works fine, as long as the only thing that gets passed around is pure, immutable data. Immutable being the weight bearing word here.

Then it came time to think about how basic basic events like a mouse click are propagated through the Circuit. In previous versions, we had taken the Qt approach of using dedicated event objects that were passed to each applicable Widget in draw stack-order. Widgets higher up would receive the event first and lower Widgets later. Every Widget could "accept" the event, which would set a flag on the event object itself notifying later Widgets that the event was already handled. This way they could act differently (or not at all), depending on whether the event has already been accepted or not. This of course requires the given event to be mutable, which isn't possible with Values.

Here a (hopefully) complete list of possible solutions to the problem:
1. `Receiver.on_next` could return a bool to let an external scheduler know if it should continue propagating the event.
2. `Receiver.on_next` could return the (optionally modified) Value to propagate further.
3. Add a mutable extra-argument to `Receiver.on_next`, so the Value itself remains immutable.
4. Have a global / Logic / Scene state for the currently handled event that can be modified.
5. Allow non-Value propagation and simply propagate classic event objects.
6. Make event handling a separate "thing" from the Logic, effectively sidestepping the problem.

### Discussion:

1. Easy, but does not allow accepted Events to be propagated. If you want a foreground button and a nice background effect whenever you click on the button, the button must know it and return "false" even though the event was accepted by it, just so that the background still receives the event. If another button moves in between the first and the background, it will not know that the event was accepted. Terrible. Declined. 

2. This is actually really interesting... You could, for example, strip or add modifier keys to a mouse click on a part of a Widget that lies on top of another one, so the lower Widget would then receive the modified Event instead. This is truly general and since Values are shared pointers anyway, should also be really cheap to return unmodified values. And it's not like you can return just anything, a Value can only be modified within its Schema so the "type" of Value will stay the same. However, it is really easy to mess around with the event handling to a point where pinpointing errors could become a hassle. There is no mechanism protecting accepted events to become unaccepted again since Values do not encode any internal logic. Additionally, this would have to be the general case, where each Receiver would need to return the Value even if no event handling is happening. And since Receivers area allowed to modify the order of their Receivers, passing on a potentially modified value from one to the next is a debugging nightmare.

3. This would work. In addition to the Value, every Emitter emits a structure that has mutable state, like whether or not the event was accepted. It could also contain additional immutable information, for example the identity of the Emitter that emitted the value or whatever. It would be nice to generalize this approach though, so we don't have to encode event handling into the Circuit. Previously, this was part of the event class itself, which is nice because if you want a different kind of behavior (maybe an event that can actually be blocked), you just write it into the specific event type. But maybe accepted or not is general enough?

4. Possible, but each with drawbacks. A global state is easily accessible but does not work with multiple instances. Logic and Scene both require the Receiver to keep a reference to them, even though they are hardly ever used.

5. I wanted to avoid that in the general case, because it would be nice to a have a Logic DAG consisting of only data flow, no additional logic encapsulated within the data, only in the Switches. Then again, allowing other types than Values to be passed isn't really a "new" concept, it is more a generalization of an existing one. It won't even change the behavior much, because Schemas act as quasi-typing for Values and so you were never allowed to just connect any two arbitrary Emitter/Receiver pairs anyway. 

6. Intriguing, but this would add yet another concept whereas we were trying to reduce the number of different concepts that make up the system.

### Decision:

This was a tough one. We stared out liking option 3 a lot, then number 2 until we realized how brittle that would be, then switched to number 5 as the "sane" default option that only required us to give up the "impossible" idea of building the Logic using only Values and Circuit elements. However, We have since gotten around to favor number 3 again, hopefully for good this time.
<br> Here is the problem with number 5: It is probably the best solution for this special case. Handling a well-defined (in this case a mouse-click) problem by introducing a new, well defined type is a straight-forward approach for every object oriented programmer. But it requires Receivers and Emitters that are both aware of the type and know how to work with it. This if fine in this example alone, but the more we thought about it, the more we realized that we would need a truly general solution.

To illustrate the problem, let's take the simple example from above: a simple mouse click should be propagated to all Widgets that are interested in its effects. To make things interesting, the click can either be a single mouse click or the second click in a double-click event! Here is the Application Logic that we originally envisioned:

```
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
```

At the very left of the Circuit, there is the simple Fact that a mouse click has occurred alongside some additional information like the position of the cursor and the button that was clicked. Whenever a click happens, it flows immediately into the "Click Counter" Switch that counts the number of clicks within a given time in the past and adds that number to the Click Value. Downstream of the counter, we have two filters that check the click count number, strip it from the output Value and fire only if it is a single click or a second click respectively. Both filters then flow into a dedicated Distributor each, which turns the Click Value into a (Double) Click Event and propagates it to the interested Widgets according to their draw order. Widget 1 is only interested in single click events, Widget 3 only in double-clicks and Widget 2 in both.

Looks good so far. We could even easily add a triple-click event and wouldn't have to change the existing network at all. What this story misses however, is that we don't get "click" messages, we only get "mouse up" messages. This only translates into a "click" if the mouse wasn't moved while it was down ... or is it a "click" regardless? What if the mouse was held down for a significant amount of time? Maybe to select a span in a playing audio file? Is this a click as well?

The problem is that there are many different ways to interpret a system event like a "mouse up" and not all of them are as cleanly cut as "is this the second click in the last x milliseconds". In the example, there was a dedicated Switch to count clicks. This seems fairly general. But what about an Switch to decide whether this was a click, a gesture, etc? Suddenly, this Switch has a lot of logic inside, written as code. This is not the general solution that we want. 

What we need here is an "Ordered X-OR" Switch, or however you want to call it. It is a way to make sure that of n different Operators, at most one will actually generate a Value - or in terms of the example: a mouse up event is either a click, the end of a drag, the end of a gesture or something else. But it should not be more than one of these things. The Switch keeps its Receivers ordered in a list and the first Receiver to report that it has successfully accepted the Value is the last one to receive it... sounds familiar. So maybe we should add another type, something like "Pre-Event", since at this point, the "mouse up" isn't yet what we called an Event earlier? Of course, we'd need a dedicated Switch to spit out these "Pre-Event" values and all of the Receivers need to accept them.
<br> ... At this point, we realized that option number 5 was a dead end as well.
<br> Which finally brings us to:

### The Fat Signal

Early on we decided that Emitters should always pass their `this` pointer alongside the emitted Value as a second argument for identification purposes only. This way the Receiver was free to sort values from different Emitters into different Buckets for example, and concatenate all of them when each Emitter had completed. There was no obvious better way to implement this feature and it seems like a straightforward, easy and cheap thing to do.

With the need for some kind of feedback from the Receiver back to the Emitter, there was yet another use-case for meta data to accompany any emitted value. It will be ignored by some/most, but offers indispensable features to others. And instead of adding another argument to the `Receiver.on_next` function, it seemed reasonable to replace Value/Id-pair with a mutable reference to what we shall call the (fat) *Signal*. _Fat_, because in addition to the Value, it encodes additional information for the Circuit. 

It consists of 3 things:
1. The constant **Value** of the Signal.

2. A **list of Emitter** IDs that describes the complete path of the Signal through the Circuit. These IDs are unique and constant, for example the memory address of the Emitter or some other integer. When we formalized the Circuit design and decided that it must be a DAG, the single Emitter ID was changed to a list of Emitter IDs, ordered so that Emitters further upstream come before those further downstream. This way makes it trivial to find the latest Emitter and to recreate the path of the Signal for debugging purposes. You can also quickly check for cyclic dependency errors at runtime, by comparing a potential Receiver/Switch to the each entry in the list and fail, if it is already present. 

3. An enum that describes the **Status** of the Signal, encapsulated in an object to ensure that it follows the state transition diagram outlined below:
    ```
    --> Ignored --> Accepted --> Blocked 
            |                       ^
            +-----------------------+
            
    --> Unblockable
    ```
    The Status always starts out as either "Ignored" or "Unblockable", with "Unblockable" being the default. An unblockable Signal cannot be stopped and calls to `accept()` or `block()` are ignored. If the Status starts out as "Ignored", then each Receiver is free to mark the Status as accepted or blocked. An accepted Signal is usually propagated further (depending on how the Emitter chooses to interpret what "accepted" means in its specific use-case), whereas the Receiver that marks its Signal as being blocked is guaranteed to be the last one to receive it.  

Signals cannot be manually created or copied, you can only "derive" a new Signal from an existing one by 
... actually, how *do* we ensure that the Signal Path is always updated? Even if a node Property is changed by a callback from a Slot that ignores the Signal completely?...
# TODO: CONTINUE HERE


## Exception Handling

With the introduction of user-written code, we inevitable open the door to user-written bugs. Therefore, during Event handling evaluation, all Receivers have the possibility of failing at any time. Failure in this case means that theReceiver throws an exception instead of returning normally. Internal errors, that are caught and handled internally,remain of course invisible to the Logic.

In case of a failure, the exception thrown by the Receiver is caught by the Emitter upstream, that is currently in the process of emitting. The way that the Emitter reacts to the exception can be selected at runtime using the [delegation pattern](https://en.wikipedia.org/wiki/Delegation_pattern). The default behavior is to acknowledge the exception by logging a warning, but ultimately to ignore it, for there is no general way to handle user code errors.Other delegates may opt to drop Receivers that fail once, fail multiple times in a row or in total, etc.
