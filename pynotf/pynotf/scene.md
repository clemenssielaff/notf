# The Scene


# Nodes and User-Code

Nodes are *the* place to build the UI through user-defined code. Of course, the code cannot be just injected anywhere, we have a specific place for it: Callbacks. A Callback is something that can be connected to an Input Plug (IPlug), the connection can be modified at run-time. This Callback receives a Value from the IPlug, but in order to work effectively, it also needs access to other members of the Node - some of which might have been user-defined as well. What are these things?

1. Properties
2. Output Plugs
3. Existing child Nodes
4. Self, in order to create or remove child nodes.

That looks like a function signature (self, ValueSignal) to me. A Callback has a name, which can be registered with the IPlug. An IPlug can only call a single or no Callback - we don't allow multiple connected Callbacks. Of course, you can set up the one Callback to call two other ones, but that's your responsibility.

## Is a Widget a Node?

The way thing are set up now, we could turn the Widget -> Node relationship from an "is-a" to a "has-a" relationship.
Meaning instead Widget deriving from Node, we could simply contain a Node as a field in Widget. Usually this is preferable, if not for one thing: discoverability. What I mean is that you can find a Node using its name in a Scene, but if the Node knows nothing about Widgets (or potential other Node-subclasses), then you don't get to a Widget from a Node. Instead, when a Widget *is-a* Node, the Node class can still be kept in the dark about the existence of a Widget class, but you will be able to `dynamic_cast` from the Node to a Widget.


# Widgets and Logic

We have now established that Logic elements (Emitter and Receivers) must be protected from removal while an update is being handled. The update process is also protected from the unexpected addition of elements, but that does not matter for the following question, which is about the relationship between logic elements and Widgets.

First off, Widgets own their logic elements. This is elementary and one of the features of notf: a Widget is fully defined by the combined state of its Properties, while Signals and Slots are tied to the type of Widget like methods are to an C++ object. But do some logic elements also own their Widget? Properties should not, they are basically dumb values that exist in the context of a Widget but are fully unaware of it. Signals do not, they are outward facing only and do not carry any additional state beyond that of what any Emitter has. Slots however can (and often will) require a strong reference to the Widget that they live on. What now if we face problem 1 from the chapter "Logic Modifications" and Widget B removes Widget C and vice-versa? 

It is clear that an immediate removal is out of the question (see above). We still need to remove them, but only when it is absolutely safe to do so. And the only time during an update process when it is safe to do so is at the very end.
At the moment, the Widget is flagged for deletion, a strong reference to it is added to a set of "to delete" Widgets in the Graph. Then, at the very end of the update, we discard every Widget that has an ancestor in the "to delete" set and delete the rest. This way, we keep the re-layouting to a minimum. 

Note that it is not possible to check whether a Widget is going to be removed at the end of the update process or not, even though such a method would be easy enough to implement. The problem is the same as with an immediate removal. 
Depending on the execution order, the flag of Widget A would either say "this Widget is going to be deleted" if B was updated first or "this Widget is not scheduled for removal" if A is updated first. The result is not wrong, but essentially useless.
The same goes for anything else that might be used as "removal light", like removing the Widget as child of the parent so it does not get found anymore when the hierarchy is traversed.


## Properties

Widget Properties act like Operators. Instead of having a fixed min- or max or validation function, we can simply define an Operations that is applied to each new input.


Dead Ends
========= ==============================================================================================================
*WARNING!* 
The chapters below are only kept as reference of what doesn't make sense, so that I can look them up once I re-discover 
the "brilliant" idea underlying each one. Each chapter has a closing paragraph on why it actually doesn't work.


## Multithreaded evaluation

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
that both Events generated independently. If however we find an Switch that:
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
be short or even ultra-short lived (by that I mean that they are emitted straight into Nirvana, without a single 
Receiver to react) and opening up a new thread for each one would be a massive overhead. On top of that, you'd have to
record CRUD and analyse the records - more overhead. And we didn't even consider the cost of doing the same work twice 
if a collision in the records is found. Combine that with the fact that we would need to keep track of *all* of the 
state and must forbid the user to manage any of her own ... I say it is not worth it.


## Property Visibility

In previous versions, Node Properties had a flag associated with their type called "visibility". A visible Property was
one that, when changed, would update the visual representation of the Widget and would cause a redraw of its Design.
I chose to get rid of this in favor of a more manual approach, where you have to call "redraw" in a Widget manually.
Here is why the visibility idea was not good:
    - Visibility only makes sense for Widgets, but all Nodes have Properties. Therefore all Nodes are tied to the 
      concept of visibility even though it only applies to a sub-group.
    - Sometimes, whether or not the Design changes is not a 1:1 correspondence on a Property update. For example, some
      Properties might only affect the Design if they change a certain amount, or if they are an even number... or
      whatever.
