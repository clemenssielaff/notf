# The Application Logic

One of the big issues that we identified with existing frameworks is the way that custom widgets tend to contain code that is not only related to the individual widget itself (how to draw it, what kind of data it stores, what interface it has to the outside world), but also application behavior. "If you click this button, then we will remove _these_ widgets, disable _these_ and create a new dialog that blocks the rest of the UI until _that_ button has also been clicked". We are not saying that this approach doesn't work, it clearly does, but it makes it hard to build a complex UI with lots of interlocking parts just because you don't always know where stuff is happening. Maybe it's happening in the button that you just clicked? Maybe the event falls through to one of the parent Widgets? Maybe it's happening in an event handler installed by a completely different widget? ... who knows? 

With notf, we wanted to make sure that the way the _entire_ application behaves is separated from the way individual elements of the UI behave. This idea resulted in the concept of the abstract "Application Logic", some way to describe the behavior of the entire application at once. One important aspect of that is that event propagation and -handling should *not* be a separate thing, whether a timer times out, the user presses a key or an asynchronous data base request has finished - everything is handled in the logic.

This text tries to document the thought processes that lead to the decisions that ultimately shaped the `logic` module. We use the term Logic here, because it describes the application behavior in deterministic [if-this-then-that terms](https://en.wikipedia.org/wiki/Logic). Note that while the Logic describes behavior in the abstract (as in: "the Logic is valid"), the classes for an actual implementation of a particular Logic use terms borrowed from signal processing and the design of [electrical circuits](https://en.wikipedia.org/wiki/Electrical_network).


## Terminology

+ **Logic**
 Describes the complete behavior space of the Application. At every point in time, the Logic is expressed through the Logic *Circuit*, but while a Circuit is mutable, the Logic itself is static. Similar to how a state machine is static, while the expressed state of the machine can change.

+ **Circuit**
 A Circuit is a concrete configuration of Emitters and Receivers arranged in a directed, acyclic graph.

+ **Event**
 An Event encompasses the introduction of a new Signal into a Circuit, its propagation and the modifications of the Circuit as a result. The Event is finished, once all Receivers have finished handling the input Signal and no more Signals are being emitted within the Circuit. Unlike in Qt, there are no "Event objects".

+ **Signal**
 Object at the front of the Event handling process. At the beginning of an Event, a single Signal is emitted into the Circuit by a single Emitter but as the Signal is propagated through, it can multiply into different Signals.

+ **Emitter**
 Is a Circuit object that emits a Signal into the Circuit. Emitters can be sources or relays of Signals, meaning they either introduce a new Signal into the circuit from somewhere outside the Logic (see Facts in the scene module) or they can create a Signals as a response to another Signal from within the Circuit. Emitters may contain a user-defined sorting function for their connected Receivers, but most will simply use the default implementation which is based on the order of connection and connection priorities.

+ **Receiver**
 Is the counter-object to an Emitter. A Receiver receives a Signal from an Emitter and handles it in a way appropriate to its type. The handler function of a Receiver is the main injection point for user-defined functionality.

+ **Operator**
 Anything that is both an Emitter and a Receiver of Signals that contains user-defined code to modify the input value. Usually maps one or more inputs to an output but note that not all Switches generate an output Signal for each input Signal all the time.


# Logic Elements

## Emitter
* Keep a list of weak references to Receivers connected downstream.
* Allow subclasses to re-order Receivers without given them write-access to the member field.
* Has an output Value type which is constant and readable.
* Has 3 protected methods: `emit`, `fail` and `complete`.
* Once `complete` or `fail` are called, the Emitter is "finished" and will never emit again.
* Has a function pointer to an exception handler that is called during emission, if a Receiver should throw.
* Has a signed recursion counter, -1 means no recursion allowed, anything else is the current recursion.
* New Receivers are appended to the list.
* Has a method to check if it has any Receivers at all.

## Receiver
* Keeps a set of strong references to Emitters connected upstream.
* Automatically disconnects from Emitters that complete or fail.
* Has an input Value type which is constant and readable.
* Constructor takes a Circuit and input Value type. 
* Has 3 public methods `on_next`, `on_complete` and `on_fail`.
* `on_next` is purely virtual.
* Can connect to existing upstream Emitters.
* Can create-connect upstream Operators.
* Connections and disconnections don't happen immediately but are queued in the Circuit.
* Stores a pointer to its Circuit. 

## Signal
* ValueSignal, 
    * Stores a Value, a state (unblockable, ignored, accepted, blocked) and a pointer to the Emitter.
    * State is the only mutable part of any Signal.
    * Constructor requires initial state, is "unblockable" by default but may also be "ignored".
    * Accepted Signals are just marked as such, blocked Signals are not propagated further.
* FailSignal.
    * Stores the exception and a pointer to the Emitter.
* CompleteSignal
    * Stores the pointer to the Emitter.

## Circuit
* Keeps a queue of delayed connections/disconnections to perform after an event has finished.
* Stores an optional function pointer with no arguments, no return value that is called after the Circuit has performed post-event cleanup.

## Operator
* Derives from Emitter and Receiver.
* Has a public, nested Subclass `Operation` with a virtual destructor and call function.
* Constructor takes the input value type, an output value type and an instance of Operation by r-value.
* Can only be created by Receivers.
* The Operation call signature takes a Signal as only argument while returning a variant of <Value, Complete, Exception>.
* The entries in the returned variant trigger the three corresponding Emitter methods.
* Before the returned Value is accepted, its schema is checked against the Emitter's output schema.

## Node

## Slot
* Is-a Receiver that forwards ValueSignals to 0-1 method of their associated Node object.
* Has a single, constant Node object associated with it.
* Which method is forwarded to can be changed by the associated Node.

## Property

## Scene
* Keeps a queue of Nodes to delete after an event has finished.
* Owns a Circuit object.
* Has a post-event cleanup method that it registers with its Circuit object.

## Fact

## Service

====

## Reactive Logic

> TODO: why did we choose the reactive component model anyway?

An experienced application programmer might find the implementation of some of these patterns familiar. In fact, what we call an _Emitter_ and _Receiver_ could just as well be called an _Observable_ and _Observer_ to follow the _ReactiveX_ naming scheme (see [reactivex.io](http://reactivex.io/documentation/observable.html)).
<br> However, these names are not very programmer-friendly so we chose to go with our own naming. Here is a comprehensive list why:
* An Observable actively pushes its value downstream even though the name implies that it is purely passive. The Observer, respectively, doesn't do much observing at all (no polling etc.) but is instead the passive one.
* The words Observable and Observer look very similar, especially when you just glance at the words to navigate the code. Furthermore, the first six characters are the same, which means that there is no way to use code-completion without having to type more than half of the word.
* When you add another qualifier (like "Value"), you end up with "ObservableValue" and "ValueObserver". Or "ValuedObservable" and "ValueObserver" if you want to keep the "Value" part up front. Both solutions are inferior to "ValueEmitter" and "ValueReceiver", two words that look distinct from each other and do not require grammatical artistry to make sense.
* Lastly, with "Observable" and "Observer", there is only one verb you can use to describe what they are doing: "to observe" / "to being observed" (because what else can they do?). This leads to incomprehensible sentences like: "All Observables contain weak references to each Observer that observes them, each Observer can observe multiple Observables while each Observables can be observed by multiple Observers". The same sentence using the terms Emitter and Receiver: "All Emitters contain weak references to each Receiver they emit to, each Receiver can connect to multiple Emitters while each Emitter can emit to multiple Receivers". It's not great prose either way, but at least the second version doesn't make me want to change my profession.

### Operators

An Operator is Receiver/Emitter that, generally speaking, maps an input value to an output value of the same or a different type (see https://en.wikipedia.org/wiki/Operator_(mathematics)).

Not every input is guaranteed to produce an output as Operators are free to store or ignore input values. The user-defined code within an Operator takes a Signal and returns a variant of [Value, COMPLETED, Exception], which map to the three emission functions. If the user-defined code returns COMPLETED or an exception, the Emitter superclass will complete or fail respectively.

Side question: do we even need user-defined code in Operators? Can't we just provide all the Operators that the user needs? Well, the reactivex libraries seem to think so. At the time of this writing, its [documentation](http://reactivex.io/documentation/operators.html#alphabetical) lists around 70 reactive components. But we would instead like our built-ins to be as minimal as possible. Sure, you can put a standard library on top and that can contain all the Operators you could think of, but you could also write your entire UI without the standard library and program all of your Operators yourself. So we *do* want user-defined code in our Operators.

### Slots

Slots are Receivers that have an associated node and than forward their received Signal to a method of a Node subclass. Which method that is depends on the state of the Node.

## Cycles in the Circuit

We started out with the assumption that the Circuit must not contain any cycles, meaning that the path that a Signal takes through the Circuit must never fold back onto itself. This is a safe requirement to make, since Emitters (either directly or through Operators or Slot callbacks) may contain user-defined code that may not be reentrant. It also rules out the possibility of the Signal cycling endlessly through the same loop, crashing the whole system. Lastly, it is also easy to prove that a graph does not contain cycles, which made the rule enforceable at runtime.
<br> So why didn't we just leave it at that?

First, we noted that reentrancy would not be a problem because by the time an Emitter emits a new Signal, all of the user-defined code has already completed. And the emission in turn does not require access to any of the user-defined state that might change in a situation where the user-defined code is executed while the emission is still running.

Next, it occurred to us that it was in fact possible to create create cyclic graphs that would never get stuck in an infinite loop because of the way their user-defined code would work. For example, an Emitter that keeps two Receivers and emits the first value it receives to the first only and the second to the second only. If the circle was through the first Receiver but not through the second, the Logic as a whole would still work fine. 

On the flip-side, the same user-defined code also makes it impossible to sort the elements in a Circuit statically (without executing it) since it might perform different based on user defined states. This meant that while it was still easy enough to prove that a Circuit did not have any cycles (if there was no cyclic connection there can not be a cycle, no matter what code the user writes), it was not possible to prove statically that Circuit *did* have cycles.

This is where we started considered allowing cycles in the graph, since the only downside was the possibility of endless loops ...though you can do that in every programming language and still people somehow manage.
An additional advantage of simply allowing cycles was that we did not have to check for them anymore. Because even though proving the absence of cycles in a graph is a simple topological sort of the graph, graphs could grow arbitrarily large (although in practice, we expect them to be in the hundreds of elements and rather shallow). Plus the fact that the Circuit has the ability to modify itself during event handling meant that we would have to re-check every time a Receiver connected to an Emitter mid-event. Quite a bit of overhead to check for something that should not happen anyway.

Then we realized that you could reliably check for cycles simply by adding an additional flag to each Emitter that is set right before emission starts and cleared immediately afterwards. If any Emitter starts emitting and finds its flag already set, we know that we have hit a loop and can throw an exception (which would be picked up by the Emitter upstream, reported and ignored).
<br> This approach allows for cheap and easy loop-detection that does not require any re-sorting when the Circuit changes. It only works at runtime, but then again the Circuit is mutable at runtime anyway, so a static solution would not be very useful.
<br> Furthermore, it opens up the possibility for restricting reentrant emission to a selected few Emitters only. Instead of a flag, we would need a flag denoting whether or not this Emitter allows reentrancy and a loop counter (or the two folded into one). You could then specify a global limit like 1000 and be certain that a true infinite loop would still be caught. 
<br> With all of that, we decided to make all Emitters not reentrant by default (meaning a loop is an immediate error), but leave it to the user to manually mark an Emitter as reentrant, thereby allowing up to GLOBAL_RECURSION_LIMIT loops. 

... And then we changed our minds and forbid it again.
<br> Here's the thing: By not allowing loops at all, we have a consistent behavior of the system that is first and foremost *safe*. There might be a use-case for loops in the future (although I doubt it) that cannot be better achieved using a DAG, but until we know of them it is a safe decision to leave those guard-rails in place. And that's what the decision is: guard rails against unexpected behavior. We have to deal with user-written code and while that is inherently unsafe, we can at least rest assured that we can always catch and stop them, before they wreck havok. And once we caught a cyclic dependency, we can inspect the topology and tell the user exactly where it is happening. 

However, finding the error does nothing to fix it. Whenever a cyclic dependency occurs in a running program and we stop it, the program will no longer work like the author indented. You could say that it never did because the author created a cyclic dependency but that statement is only true if cyclic dependencies are forbidden in general. If we allow them, then the demarkation between a valid and an invalid Circuit becomes blurry. Depending on where you set the recursion limit, the same Circuit may be valid in one case and not in the other.

Now that we don't allow changes in the Circuit mid-event, we might even go one step further and check for cyclic dependencies at the point where they are created. This has the downside that it might cause sudden spikes in event handling times (when you connect into a very deep DAG that you need to traverse all the way in order to make sure that no cycle occurs), but we *could* only enable it during debugging sessions.
<br> Actually, scratch that. There is no way we can statically detect cyclic dependencies because we don't only have Operators but also Slots with user-defined callbacks, that are free to trigger any (maybe multiple) Emitters on the Node.

This leaves the question on whether we want to catch cyclic dependency errors in release mode. It is true, that we can do nothing about errors once they have occured and I don't currently see a way to make cyclic dependencies impossible on a conceptual level ... If every Operator had a single upstream Emitter, we would force the entire Circuit to be a forest but we always have Nodes in there, that act as (transparent) upstream for Node Emitters. 
<br> I guess, we could leave that up to a compiler flag but have it enabled by default. This way the default behavior is to catch the error and report them (to the user of the application, who can then hopefully report to the programmer), but if the programmer is sure that cyclic dependencies will never occur and/or that they are okay somehow and that the speed and/or space overhead is worth it - then the checks can be disabled.

Note that we still not able to catch all infinite loops. Namely those that span more than one Event. Let's say that `A` schedules a Signal to be emitted from `B`, which schedules a Signal to be emitted from `C` and `C` then back to `A`. This way, every Event (`A`->`B`, `B`->`C` and `C`->`A`) sees every Emitter only once an has no way of detecting the cycle. This is what we would call a [livelock](https://en.wikipedia.org/wiki/Deadlock#Livelock).
<br> Then again, even though this would lead to the event loop spinning 100% of the time, it would not actually lock the system up. Since there is nothing we can do to stop the inclined developer to fall down this particular hole and the effects are annoying but not critical, we just leave it at that. If a livelock ever turns out to be an actual problem, we are sure you could detect them, maybe using heuristics and a little introspection... but until then any effort spend will not be worth it.


## Ownership and Lifetimes

While it is obvious that Emitters must store a a list of references to their connected Receivers, whether or not Receivers need to store references to their connected Emitters is a question of design requirements. Furthermore, references come in two flavors: strong and weak. Strong reference imply ownership of the referenced object, weak ones do not. In order to avoid memory leaks we need to ensure that the graph of all strong references is a DAG. The other design consideration with strong references is that of object lifetime: ideally you want an object to stay around exactly as long as it is needed but no longer. 

### External and Reverse Ownership

In the beginning, Receivers kept a set of strong references to their Emitters. The rationale for that decision assumed that there were certain fixed points in the Circuit (Facts, for example) that were kept alive from the outside. Receivers would spawn into the Circuit and with them a "pipeline" of Operators upstream, that would connect to one or more of these fixed points in order to generate a customized stream of data. Since the pipeline was tailor crafted for a Receiver, it made sense that the Receiver owned the pipeline and since a pipeline was a sequence of Operators, the obvious way to achieve this behavior was to have downstream Receivers own their upstream Emitters. 
<br> Let's call this the *reverse ownership* approach, because the ownership goes in the opposite direction of the data flow.

Then we introduced *partial Operations*, which offered an easy way to construct an entire pipeline in a single Circuit element. Instead of having n-Operators daisy-chained together, you could now have a single Operator that produces the same result (and more efficiently so, at least in a compiled language). While this did not influence the question of ownership, it did result in Circuits with far fewer Operators. Suddenly it became feasible to burden the user with keeping track of an entire pipeline of Operators, something we considered before but ultimately deemed as too involved.

Next, we started designing the *Scene* and its *Nodes*. Nodes own a set of Receivers (called *Slots*) and Emitters. Since the life time of a Node varies from the entire duration of the session down to less than a second, we had to consider the fact that Receivers would regularly outlive Nodes with Emitters upstream. Up to then, this meant that the downstream Receiver would keep a part of the Node alive, or even the entire Node, depending on how Nodes were designed. This was counter to the idea that Nodes should appear as a unit and also disappear as one. The second version of the Receiver design therefore did not own *any* references to the Emitters they were connected to. If an Emitter went out of scope, the Receiver would receive a completed or failure message and that was that. The Emitters in turn did not own their Receivers either. If a Receiver dropped, the Emitter would simply remove it and carry on. This moved the responsibility of ownership entirely outside the Circuit. 
<br> Let's call this the *external ownership* approach.

Eventually we realized that if Node Emitters would always finish when the Node was removed, and all Receivers were guaranteed to disconnect from a finished Emitter, the *reverse ownership* approach would still work and it would be generalizable, since a finished (either completed or failed) Emitter will never emit again. There is no reason to keep it around. With both models feasible, we had to settle on one.

The *external ownership* approach has the advantage of being extremely light-weight. Only the bare minimum of data is stored in the circuit and since every Operator has to be owned externally (by a Node or some other mechanism), users are encouraged to keep the number of Operators reasonably small.
<br> The same could be said of the *reverse ownership* approach, but through different means. By relieving the user of the burden of keeping Operators alive, it becomes easier to construct throw-away Operators and Emitters because they live just as long as they are needed and are automatically deleted when they have finished or have lost all of their Receivers. This could lead to more Operators, but it also encourages the re-use of existing Operators since their lifetime is no longer tied to some external instance. 

Overall, I think that the advantages of having the automatic lifetime management of the *reverse ownership* outweigh the space savings of the *external ownership* approach. Note that the *reverse ownership* does not have a runtime overhead, since the strong references from Receivers to Emitters do not take part in the propagation of Signals. And the space overhead is that of as many `shared_ptr`s as there are Emitters connected upstream...And I would suspect that that number is rather small.

### The Life of an Operator

This chapter preempts a later chapter "Logic Modification", but since this is mostly about ownership, we still decided to put it here.

The question is: who owns a Circuit element? In the previous chapter we already established that downstream Circuit elements own their upstream elements and while this is so, it is not the whole truth. Someone has to own the sink element as well, otherwise the whole chain would expire immediately. The only entity that we allow the ownership of sink elements are Nodes.

The special thing about Nodes is that they are at the same time user-definable and known to us (the notf system). The user can say "I want to create a Node with 3 integer Receivers (Slots)" and that's what we deliver. And when it comes time to delete the Node, we know exactly what Slots there are and when to delete them. This is fundamentally different from user-defined code, which is entirely unknowable to the system. If Circuit elements were free to be managed by user-defined code, they might be created and removed all over the place and we would never know.
<br> Why is that problematic? As we discuss in "Logic Modification", the creation and destruction of connections in a Circuit must not happen immediately, but instead be put into a queue and delayed until the end of the event. Without some sort of oversight of connection creation/removal in the Circuit, we cannot guarantee that events will be propagated correctly. 

At the same time, we want to enable the user to create new Operators (no Emitters or Receivers, only Operators) at runtime, for example to transform the Value of a Fact into a Value that is useful to a Slot. These Operators are not part of the Node type and are therefore unknown to us. How do we do that?
<br> Well, the Operator constructor must not be part of its public API so it remains inaccessible from user-defined code. Nodes are able to create Operators, but they don't have any place to store them and we certainly don't want to hand the Operator back to the user. So instead, we only allow Slots to create and connect to a (user-defined) Operator immediately. You can think of Operators growing out to the left of Nodes like roots, to connect to existing sources of data upstream. All the user gets back is a reference of a non-copyable, non-movable object. Of course, this won't stop the adventurous to store raw pointers to the Operator but you have to assume that these brave souls will [dilbert principle](https://en.wikipedia.org/wiki/Dilbert_principle) their way out of our user-base eventually.

The observant reader might have noticed a slight-of-hand here. While the "create-and-connect" approach is successful in creating arbitrary Operators without ever handing back ownership to the user, we are in fact creating untracked connections from user-defined code. Which is exactly what we wanted to avoid.
<br> Fortunately, this is handled internally by the Circuit elements, without the user even knowing about it. Every Receiver (a Operator is-a Receiver) has a reference back to a central `Circuit` object, which stores a queue of all new and deleted connections during an event. At the end of the event loop, all new connections are created first, then all deleted ones are removed. The order is important because an Emitter without outgoing connections would be destroyed before new connections had the chance to keep it alive.
<br> The Circuit is owned by a Scene, which is in turn referenced by the Node, which is visible from the Slot. When the Slot creates-and-connects to an upstream Operator, it can pass the Circuit as argument to the Operator's constructor. Similarly, when a Operator creates-and-connects to a new upstream Operator, it will pass its own Circuit as argument. This way, we can offer the user convenient modification functions through the downstream Circuit elements and still have complete visibility.

There is however another place in the architecture that allows user-defined code: Operators. Or actually, a functor with a default constructor and well defined "call" implementation taking a Signal and returning a Variant... And that is already the end of the discussion as Operations have no more access to their containing Operators than they have access to any other Operator in the Circuit.
 

## What is in a Signal

Early on, we decided that the only thing to emitted by Emitters should be Values, instead of built-in or user-defined types. This would allow complete introspection of the system in any environment, and would allow the seamless interaction of Circuit elements defined in compiled languages like C++ and interpreted ones like Python. This approach works fine, as long as the only thing that gets passed around is pure, immutable data. Immutable being the weight bearing word here.

Then it came time to think about how basic basic events like a mouse click are propagated through the Circuit. Let us assume for now, that we have a mechanism in place to propagate an event (however structured) to each applicable Slots in draw stack-order of their associated Widget. There is a whole chapter on this topic alone (see Ordered Emission)
<br> In previous versions, we had taken the Qt approach of using dedicated event objects. Widgets higher up would receive the event first and lower Widgets later. Every Widget could "accept" the event, which would set a flag on the event object itself notifying later Widgets that the event was already handled. This way they could act differently (or not at all), depending on whether the event has already been accepted or not. This of course requires the given event to be mutable, which isn't possible with Values.

Here a (hopefully) complete list of possible solutions to the problem:
1. `Receiver.on_next` could return a bool to let an external scheduler know if it should continue propagating the event.
2. `Receiver.on_next` could return the (optionally modified) Value to propagate further.
3. Add a mutable extra-argument to `Receiver.on_next`, so the Value itself remains immutable.
4. Have a global / Logic / Scene state for the currently handled event that can be modified.
5. Allow non-Value propagation and simply propagate classic event objects.
6. Make event handling a separate "thing" from the Logic, effectively sidestepping the problem.

### Discussion:

1. Easy, but does not allow accepted Events to be propagated. If you want a foreground button and a nice background effect whenever you click on the button, the button must know it and return "false" even though the event was accepted by it, just so that the background still receives the event. If another button moves in between the first and the background, it will not know that the event was accepted. Terrible. Declined. 

2. This is actually really interesting... You could, for example, strip or add modifier keys to a mouse click on a part of a Widget that lies on top of another one, so the lower Widget would then receive the modified Event instead. This is truly general and since Values are shared pointers anyway, should also be really cheap to return unmodified values. And it's not like you can return just anything, a Value can only be modified within its Schema so the "type" of Value will stay the same. 
<br> However, it is really easy to mess around with the event handling to a point where pinpointing errors could become a hassle. There is no mechanism protecting accepted events to become unaccepted again since Values do not encode any internal logic. Additionally, this would have to be the general case, where each Receiver would need to return the Value even if no event handling is happening. And since Receivers area allowed to modify the order of their Receivers, passing on a potentially modified value from one to the next is a debugging nightmare.

3. This would work. In addition to the Value, every Emitter emits a structure that has mutable state, like whether or not the event was accepted. It could also contain additional immutable information, for example the identity of the Emitter that emitted the value or whatever. It would be nice to generalize this approach though, so we don't have to encode event handling into the Circuit. Previously, this was part of the event class itself, which is nice because if you want a different kind of behavior (maybe an event that can actually be blocked), you just write it into the specific event type. But maybe accepted or not is general enough?

4. Possible, but each with drawbacks. A global state is easily accessible but does not work with multiple instances. Logic and Scene both require the Receiver to keep a reference to them, even though they are hardly ever used.

5. I wanted to avoid that in the general case, because it would be nice to a have a Logic DAG consisting of only data flow, no additional logic encapsulated within the data, only in the Operators. Then again, allowing other types than Values to be passed isn't really a "new" concept, it is more a generalization of an existing one. It won't even change the behavior much, because Schemas act as quasi-typing for Values and so you were never allowed to just connect any two arbitrary Emitter/Receiver pairs anyway. 

6. Intriguing, but this would add yet another concept whereas we were trying to reduce the number of different concepts that make up the system.

### Decision:

This was a tough one. We stared out liking option 3 a lot, then number 2 until we realized how brittle that would be, then switched to number 5 as the "sane" default option that only required us to give up the "impossible" idea of building the Logic using only Values and Circuit elements. However, We have since gotten around to favor number 3 again, hopefully for good this time.
<br> Here is the problem with number 5: It is probably the best solution for this special case. Handling a well-defined (in this case a mouse-click) problem by introducing a new, well defined type is a straight-forward approach for every object oriented programmer. But it requires Receivers and Emitters that are both aware of the type and know how to work with it. This if fine in this example alone, but the more we thought about it, the more we realized that we would need a truly general solution.

To illustrate the problem, let's take the simple example from above: a simple mouse click should be propagated to all Widgets that are interested in its effects. To make things interesting, the click can either be a single mouse click or the second click in a double-click event. Here is the Application Logic that we originally envisioned:

```
                                             +---------------+
                                             |    Filter     |
                                             | Single Click  |
                                             | ------------- |    +-------------- +
                                          +---> Click Value  |    |  Distributor  |
    +-------------+    +---------------+  |  |   & count     |    | Single Click  |
    |    Fact     |    |    Operator   |  |  | ------------- |    | ------------- |
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

At the very left of the Circuit, there is the simple Fact that a mouse click has occurred alongside some additional information like the position of the cursor and the button that was clicked. Whenever a click happens, it flows immediately into the "Click Counter" Operator that counts the number of clicks within a given time in the past and adds that number to the Click Value. Downstream of the counter, we have two filters that check the click count number, strip it from the output Value and fire only if it is a single click or a second click respectively. Both filters then flow into a dedicated Distributor each, which turns the Click Value into a (Double) Click Event and propagates it to the interested Widgets according to their draw order. Widget 1 is only interested in single click events, Widget 3 only in double-clicks and Widget 2 in both.

Looks good so far. We could even easily add a triple-click event and wouldn't have to change the existing network at all. What this story misses however, is that we don't get "click" messages, we only get "mouse up" messages. This only translates into a "click" if the mouse wasn't moved while it was down ... or is it a "click" regardless? What if the mouse was held down for a significant amount of time? Maybe to select a span in a playing audio file? Is this a click as well?

The problem is that there are many different ways to interpret a system event like a "mouse up" and not all of them are as cleanly cut as "is this the second click in the last x milliseconds" *or not*. In the example, there was a dedicated Operator to count clicks. This seems fairly general. But what about an Operator to decide whether this was a click, a gesture, etc. especially if it could be multiple of these things? What if you need to decide which of the things should be propagated to the target Widget? Suddenly, this Operator (and the event object it produces) has a lot of logic inside, written as code. for This is not the general solution that we want.

What we need here is an "Ordered X-OR" Operator, or however you want to call it. It is a way to make sure that of n different Operators, at most one will actually generate a Value - or in terms of the example: a mouse up event is either a click, the end of a drag, the end of a gesture or something else. But it should not be more than one of these things. The Ordered X-OR Operator keeps its Receivers ordered in a list and the first Receiver to report that it has successfully accepted the Value is the last one to receive it... sounds familiar. So maybe we should add another type, something like "Pre-Event", since at this point, the "mouse up" isn't yet what we called an Event earlier? Of course, we'd need a dedicated Operator to spit out these "Pre-Event" values and all of the Receivers need to accept them.
<br> ... At this point, we realized that option number 5 was a dead end as well.
<br> Which finally brings us to:

### The Fat Signal

From the start we decided that Emitters should always pass their `this` pointer alongside the emitted Value as a second argument for identification purposes. This way the Receiver was free to sort values from different Emitters into different Buckets for example, and concatenate all of them when each Emitter had completed. There was no obvious better way to implement this feature and it seems like a straightforward, easy and cheap thing to do.

With the need for some kind of feedback from the Receiver back to the Emitter, there was yet another use-case for meta data to accompany any emitted value. It will be ignored by some/most, but offers indispensable features to others. And instead of adding another argument to the `Receiver.on_next` function, it seemed reasonable to replace Value/Id-pair with a mutable reference to what we shall call the (fat) *Signal*. _Fat_, because in addition to the Value, it encodes additional information for the Circuit. 

It consists of 3 things:
1. The constant **Value** of the Signal.

2. An **Emitter ID** that unique identifies the upstream Emitter that produced the Signal. IDs are unique and constant, for example the memory address of the Emitter or some other integer. Signals with an ID of another Emitter than the Receiver is connected to should trigger an assert.

3. An enum that describes the **Status** of the Signal, encapsulated in an object to ensure that it follows the state transition diagram outlined below:
    ```
    --> Ignored --> Accepted --> Blocked 
            |                       ^
            +-----------------------+
    --> Unblockable
    ```
    The Status always starts out as either "Ignored" or "Unblockable", with "Unblockable" being the default. An unblockable Signal cannot be stopped and calls to `accept()` or `block()` are ignored. If the Status starts out as "Ignored", then each Receiver is free to mark the Status as accepted or blocked. An accepted Signal is usually propagated further (depending on how the Emitter chooses to interpret what "accepted" means in its specific use-case), whereas the Receiver that marks its Signal as being blocked is guaranteed to be the last one to receive it.  

Note that the "completion" Signal only contains the Emitter ID, while the "failure" Signal contains the Emitter ID and the exception.

This of course leaves the question of where to decide whether a Signal is blockable or unblockable. Our initial assumption was that the Emitter contained a virtual method in which the programmer would just create the Signal to emit and then push it to a protected emission function.
<br> Turns out, we don't really want that. We don't want to let the user create the Signal since that would allow modifying the Value or passing another Emitter as source. We also don't want to ask the user to handle the emission, especially the error-handling part. And if a Signal is blockable, then it must really be blocked (which means, no user-code deciding whether it is blocked or not). The only part that has to be decided by the user is whether or not a Signal is blockable and the order in which the emission takes place.
<br> Let's put the ordering aside for now. There are two possibilities: either whether or not a Signal is blockable is part of the Emitter type or it can change due to the user-defined state of the Emitter. But wait, Emitters don't really have user-defined state, do they? Operators do, but non of that state is allowed to influence the emission process to allow reentrancy (even though we forbid cycles, but that's an orthogonal decision). Actually, that doesn't matter because the Emitter could still decide based on the Receivers that it emits to whether or not the Signal is blockable... but it shouldn't. I imagine looking at the Circuit in a graphical representation and I want to know which Emitters emit blockable Signals and which don't. I certainly don't want any "sometimes yes, sometimes no", just as I don't want that with recursive functions (sometimes they cycle, sometimes they don't).
Therfore, whether or not a Signal is blockable depends on the type of Emitter and must not change, which means that we need to set it once inside the Emitter constructor and then leave it. I ran a benchmark (http://quick-bench.com/76pAs0xDDThJ45qwKwu0v0KvlDI) comparing a stored boolean to a virtual function call and while I am pleased to report that there is absolutely no difference between the two (because of 100% correct branch prediction I guess?) it occurred to me that a virtual function is actually not useful if we want to ensure that we get the same result every time.


## Exception Handling

With the introduction of user-written code, we inevitable open the door to user-written bugs. Therefore, during Event handling evaluation, all Receivers have the possibility of failing at any time. Failure in this case means that the Receiver throws an exception instead of returning normally. Internal errors, that are caught and handled internally, remain of course invisible to the Logic.

In case of a failure, the exception thrown by the Receiver is caught by the Emitter upstream, that is currently in the process of emitting. The way that the Emitter reacts to the exception can be selected at runtime using the [delegation pattern](https://en.wikipedia.org/wiki/Delegation_pattern). The default behavior is to acknowledge the exception by logging a warning, but ultimately to ignore it, for there is no general way to handle user code errors. Other delegates may opt to drop Receivers that fail once, fail multiple times in a row or in total, etc.


## Logic Modification

This chapter is best read together with the next one "Ordered Emission", as they overlap quite a bit and each assume decisions described in the other one.

One thing that is quite unique to the Circuit is that it allows the for modification of itself by user-code while an Event is being processed. We have pondered whether or not this is a bug or a feature and have come up with quite a few thoughts on the topic.

There are only 4 basic operations that can happen in the Circuit, all together forming the acronym CRUD:

* *Create* a new connection between Circuit elements
* *Read* a value from state stored somewhere in the Circuit
* *Update* the combined state of the Circuit (any variable in any of the elements)
* *Delete* a connection between Circuit elements

Of course you can also create and delete circuit elements, but an element without a connection is of no consequence to the Logic expressed through the Circuit. Therefore we focus on the creation and deletion of connections.

First off, reading the state of the Circuit does not pose any problem whatsoever. This is an easy one.

Updating any value within the Circuit is also trivial. Whenever an Operator gets a new Value from a Signal, it should be able to use it to update its own state before deciding how, to whom and if to emit a new Signal to the connected Receivers. Updates must be immediate to have any meaningful impact on the Circuit.

Creating and Deleting connections however is where we started to encounter non-trivial questions. Let's take the following toy problem:

```
    A +-------> B
      | 
      +-(new)-> C
```

Let `B` be a Slot of a Widget that reacts to mouse up Signals emitted from `A`. Whenever the user clicks into the Widget, `B` will create a new child Widget with new Slot `C` that also connects to `A`. When `C` is called, the new child Widget will hide.
<br> The problem arises since `A` is still emitting and `C` is later in the list than `B`, `C` will receive the same mouse-up event immediately and hide the child Widget... which will effectively never appear, no matter how ofter `B` is clicked.

Another problem deals with a similar problem, this time deleting a connection:

```
         +---> S1
    E1 --+
         +---> S2
```

Emitter `E1` is connected to downstream Operators `S1` and `S2`. As Operators are allowed to hold state and execute user-defined code, it is trivial to set them up in a way where `S1` deletes `S2` and `S2` deletes `S1`. Whoever is emitted to first deletes the other one. 

There are multiple ways that these problems could be addressed:

1. Have a Scheduler determine the order of all updates before beginning the Event processing. During the scheduling step, all expired Receivers are removed and a list of strong reference for all live Receivers are stored in the Scheduler to ensure that they survive (which solves problem 2). Additionally, when new Receivers are added to any Emitter during the update process, they will not be part of the Scheduler and are simply not called (solves problem 1).

2. When an Emitter iterates through its Receivers to propagate a Signal, it should create a list of weak references to its Receivers first and iterate that instead of the member field. All changes to the list of Receivers would occur in the member list and not affect the emission process.

3. Do not allow the direct addition or removal of connections and instead record them in a buffer. This buffer is then executed at the end of the update process, cleanly separating the old and the new DAG state.

Solution 1 would require the introduction of a heavy new concept, the scheduler. It would also require a two-phase Event processing: in the first phase we would simply establish the order in which Signals are emitted from where to where, and then the second phase where the propagation actually takes place. Since we have a lot of user-code within Operators, there is no way to statically determine the order without executing dedicated code for each phase. That means that each Operator now has two instead of one user-defined function.
<br> All of this sounds like a last-resort measure and a quite hacky one at that.

Solution 2 is the one that we stuck with for the longest time. Basically what it does is fix the list of iteration for each active Emitter. Whenever a new connection is added, it will not show up in the list of Receivers to emit to, mitigating the first problem. And you can ignore the second problem, if you accept that the order in which Signals are sent is part of the Circuit and therefore the responsibility of the user (see the next chapter: "Ordered Emission"). In that case, `S1` would always receive the Signal first and remove `S2`. Since the emission keeps a list of *weak* references only, when it comes time to emit to `S2`, it will already have been deleted. No ambiguity left. 

Solution 3 is an interesting one. What it means is that no matter if you add or delete a connection, the effect of the change will not influence the current Event handling at all. Only the next event will pass through the modified Circuit. This might sound unnecessarily complicated at first, but instead it comes with a few very nice guarantees:

* You can always be sure that a newly created connection will not be used during the current event. With solution 2, this is not guaranteed. It is deterministic, but hard to design correctly as only the *currently active* Emitters have a fixed list of Receivers. All others do not. This allows for the following scenario:

  ```
           +---> S3
      E2 --+
           +---> S4 -(new)-> R1
  ```
  
  Let's say `E2` emits to `S3` and `S4` in that order. `S3` creates a new Receiver `R1` and connects it to `S4`. Since `S4` is not currently emitting, it will update its list of Receivers and once the Signal from `E2` arrives, `S4` will immediately emit to `R1`.  
  <br> Now, let's say that instead of the order above, `S4` happened to connect to `E2` one event loop *before* `S3`:

  ```
           +---> S4 -(new)-> R1
      E2 --+
           +---> S3 
  ```
  
  In this case, every Operator will work exactly the same, but since `S3` receives the Signal *after* `S4`, `R1` will not receive a Signal this time. Note that the order in which connections are made is still in the hand of the user so technically, both cases are "correct". However, especially if S3 is a Circuit Element that is removed and added back in on a regular basis (maybe a Widget in a Dialog that opens and closes regularly?), this will make for some hard-to-debug cases where the act of removing and immediately recreating a Operator can vastly alter the behavior of the Logic.

* You can be sure that if a Receiver `R` is connected to an Emitter and is alive at the time that that Emitter emits an (unblockable, as is the default) Signal - the `R` will get it eventually, even if some earlier Receivers decide to delete `R`. The reason that is good is the same as in the example above. Maybe you don't know the rest of the state of the UI at any point. Maybe there is another Receiver that wants to delete `R` and depending on how the user clicked through the interface it might have either connected before or after `R` was connected. We will discuss another solution to this problem later on (emission priorities) but they might not be applicable in all situations.

So, solution number 3 it is. In order for this to work, all Circuit elements must store a reference to their Circuit or some other central instance to register connection additions and deletions. We considered to pass the Circuit as a part of the Signal instead, that would introduce quite a bit of overhead passing the Circuit reference from one Signal to the next. Also, Circuit elements *are* part of the Circuit, it is only natural for them to keep a (non-owning) reference.


## Ordered Emission

Originally, we did not assume any order in the emission process. Conceptually, the Signals should flow through the Circuit like they do in a real electrical circuit: all at the same time. One important property of this approach was, that the order in which Receivers were connected to Emitters did not matter. In a way, you could say that Circuit element was to be [commutative](https://en.wikipedia.org/wiki/Commutative_property). This was important because the same set of (functionally unrelated) connections might have a completely different effect on the overall Logic, if their order happened to be different (see previous chapter on "Logic Modification")
<br> Since we don't actually employ parallelism during Event handling, the computer needed some (hidden) order to do things, but that should not matter. We called it "essentially random", meaning it wasn't really random, but it might as well have been and if someone would implement an Emitter that used actual randomness to shuffle its Receivers around, then that was fine by us.

It was a solid ideal, but we soon found the first potential problems that arose from introducing "essential randomness" into the event handling:
<br> Given one Emitter `E1`, two Operators `A1` (that always adds two to the input number) and `A2` (that always adds three) and a special Operator `S1` that is connected to both `A1` and `A2`. The Circuit is laid out as follows:

```
          +---> A1 (adds 2) ---+
          |                     \
    E1 ---+                      S1 
          |                     /
          +---> A2 (adds 3) ---+
```

The behavior of `S1` is that it receives and stores two numbers into its local state `x` and `y` and after receiving the second one, produces `z` with `z = x - y`. It then resets and waits for the next pair. 
<br> The problem is that if `E1` propagates the number one first to either `A1` or `A2` (because, as of now the order is essentially random), `x` will either be 3 or 4, while `y` will be the other one. That results in either `z = 3 - 4 = -1` or `z = 4 - 3 = 1`. Which one we get is as random as the order of propagation from `E1`.

Therefore, we decided to get rid of randomness and guarantee that the order in which Emitters emit Values to their Receives is deterministic. Note that that does not mean that the order must be fixed. It can be dynamic, for example determined by the z-value of the Nodes attached to Slots. What it does mean however is that the order must be replicable, inspectable and modifiable by the user. All of the problems introduced through randomness, including the Node removal mid-event, would be solved if we place the responsibility of the order in which Signals are emitted on the user. 
<br> Question is, how do we do that?

The only way to relate independent Receivers to each other is through a common ranking system. Fortunately there is one: integers. Whenever a Receiver connects to an Emitter, it has the opportunity to pass an optional priority integer, with higher priority Receiver receiving Signals before lower priority Receiver. The default priority is zero. Receivers with equal priority receive their Signals in the order in which they subscribed. This approach allows the user to ignore priorities in most cases (since in most cases, you should not care) and in the cases outlined above, the user has the ability to manually determine an order. The user is also free to define a total order for each Receiver, should that ever become an issue.

> I don't think we need that anymore, now that connection/disconnection is delayed until the end of the event. The example outlined above is just not very good code.

New Receivers are ordered behind existing ones, not before them. You could make a point that prepending new ones to the list of Receivers would make sure that new Receivers always get the Signal and are not blocked, but we think that argument is not very strong. A good argument for the other side is that appending new Receivers to the end means that the existing Logic is undisturbed, meaning there is as little (maybe no) change in how the circuit operates as a whole.
<br> That should be the default behavior. 

Of course, this approach does not protect the user from the error cases outlined above - but it will make them at least deterministic. I think it is a good trade-off though. By allowing user-injected, stateful code in our system, we not only give power to the user but also responsibility (cue obligatory Spiderman joke).

### Dynamic Order

Emitters are free to choose the order in which they emit to their connected Receivers and have a virtual function to do so. That also means that they are free to interpret emission priority how they see fit, as long as Receivers with a higher priority are guaranteed to receive a Signal no later than ones with a lower one.
