# The Scene


# The Node

* Have IPlugs and OPlugs
* Have Properties
* Have a state machine
* Have a private data Value
* Have a dict of named Callbacks
* Have a single, constant and always valid parent Node (except the Root)
* Have 0-n children in a data structure that can produce a strong order of children
* Have a name, that is unique in their parent node
* Can be accessed using absolute or relative paths

Nodes have input and output plugs, special Receiver and Emitters that that allow the Node to communicate with the rest of the Circuit. Input Plugs (IPlugs) take a ValueSignal and ignore all others (apart from disconnecting, which a Receiver does automatically). They then forward the ValueSignal to a Callback inside the Node. Which callback they forward to can be changed at runtime as part of the Node's state machine.

Nodes can only be created by a parent and only be removed by a parent. When a parent removes a Node, it will first remove all of its children in turn before calling user - defined or any other self - removal code. This way we can guarantee that child nodes sways have a valid, fully initialized parent for their entire lifetime.

Properties are basically Operators with a state-less Operation.

## Node Paths

Instead of a central name register, every node has a path. This way, you can request child nodes in user-code or any node in the graph that you know the absolute or relative path to, actually - plus if the path is a string it can be put into a value

IPlugs, OPlugs and Properties share the same namespace, each can be addressed as part of the node using the syntax "/first_ancestor/.../node:property" (or :iplug, :oplug)

Nodes cannot be reparented
Imagine they could, and you had an Emitter that sorts receivers by depth using a quad tree. Every time you iterate, you'd need to rebuild the tree - because you would never know whether an existing receiver had changed its location in the scene. That's quite a bit of overhead. 
This also means, you cannot move nodes in the list of children of the parent once they are in there. You can insert new nodes in between but the relative order between existing siblings must not change

## Nodes and User-Code

Nodes are *the* place to build the UI through user-defined code. Of course, the code cannot be just injected anywhere, we have a specific place for it: Callbacks. A Callback is something that can be connected to an Input Plug (IPlug), the connection can be modified at run-time. This Callback receives a Value from the IPlug, but in order to work effectively, it also needs access to other members of the Node - some of which might have been user-defined as well. What are these things?

1. Properties
2. Output Plugs
3. Existing child Nodes
4. Self, in order to create or remove child nodes.

That looks like a function signature (self, ValueSignal) to me. A Callback has a name, which can be registered with the IPlug. An IPlug can only call a single or no Callback - we don't allow multiple connected Callbacks. Of course, you can set up the one Callback to call two other ones, but that's your responsibility.

## Private Node Data

Original question: *Should properties always emit a pair of old/ new value?*
 
The reason why I thought this might be a good idea is that otherwise you could have functions that require both values, for example to check if the value changed from positive to negative or whatever. If you only provide the one value, the Node must store the previous value somewhere. 

Then again, I guess we could just allow a single Value as state of the Node, one that is not a Property and is private to the Node itself. 

## Node State Machine

### State Switching

The question right now is if Receivers always call their new callbacks right away or not - and if they do, in which order?

My first instinct was to say "Whenever a Callback is reassigned, the new one is called immediately with the latest Value". Then I realized, that during a State change this would mean that the first Callback is called before the last Callback in the State change has a change to switch leading to weird race-conditions where one Callback somehow triggers others (through Properties) even though they are currently undergoing a State change.
The next step was to say, after a State change, all new Callbacks are called once, this way we don't have any in-transition calls. This sounds like it is required for a working system but not sufficient yet. After all, the order in which the Callbacks are called is also important. Ideally, it shouldn't but I am not sure if I can rely on orthogonality between Callbacks.

Actually, I can be sure that they are not orthogonal. What about a Node that has different Callbacks for "mouse down" and "mouse up"? Even though it isn't specified anywhere in the Node, it is reasonable to expect "mouse down" to be the first call and a corresponding "mouse up" before the next one.

Maybe then, this idea of re-emitting Values from IPlugs and Properties is a bad idea to begin with? 
Yes it is.
Instead, a State change should go like this:

1. StateA:Enter function is called. StateA is the entry State of the Node.
2. Somehow a transition to StateB is triggered.
3. StateA:Exit function is called
4. StateB:Enter function is called

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

In previous versions, Node Properties had a flag associated with their type called "visibility". A visible Property was
one that, when changed, would update the visual representation of the Widget and would cause a redraw of its Design.
I chose to get rid of this in favor of a more manual approach, where you have to call "redraw" in a Widget manually.
Here is why the visibility idea was not good:
    - Visibility only makes sense for Widgets, but all Nodes have Properties. Therefore all Nodes are tied to the 
      concept of visibility even though it only applies to a sub-group.
    - Sometimes, whether or not the Design changes is not a 1:1 correspondence on a Property update. For example, some
      Properties might only affect the Design if they change a certain amount, or if they are an even number... or
      whatever.