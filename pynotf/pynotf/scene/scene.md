# The Scene

The Scene takes its name from the "Scene Graph" that is inherit in many game engines or DCC tools. It is older than the Circuit in its conception but has now been delegated to serve "only" ownership and management functionality.  The Scene is conceptually somewhat orthogonal to the Logic, but the Scene module depends on the Logic module and not the other way around.

Basically, a Scene is a tree of Widgets. There is one root Widget in a Scene, and like every other Widget, it can have 0-n children. Every Widget has a single parent that outlives it and cannot be changed. When the Scene is removed, it will safely destroy all of the Widgets from the leaf Widgets up, meaning that the root is the last to go.

A Scene owns a Circuit and manages it in a way that allows the user to inject user-defined code at very specific places that are as expressive as possible while still being safe. All modification of a Scene (and its Circuit) is confined to the Logic thread and is assumed to be single threaded. You can have parallel loops or similar approaches where they make sense, but they must operate on local data only. Much like Circuit Elements, the removal of Widget in the Scene is delayed until the end of an Event.

# Quick Facts

## Scene
- [X] Owns the root of the Widget hierarchy and safely deletes it up on destruction.
- [X] Owns the Circuit that contains all Widgets' Circuit Elements (Input/Output Plugs and Properties).
- [X] Keeps a queue of Widget.Handles to delete after an event has finished.
- [X] Can create first-level Widgets that are direct children of the root.
- [X] Can safely remove all expired Widgets, with children deleted before their parents.
- [ ] Can find and return Widgets using absolute paths

## Widget
- [X] Are constructed from a Widget.Type object
- [ ] Have Input and Output Plugs
- [ ] Have Properties
- [ ] Have a state machine
- [ ] Have a private data Value
- [ ] Have a dict of named Callbacks
- [ ] Have a single, constant and always valid parent Node (except the Root)
- [ ] Have 0-n children in a data structure that can produce a strong order of children
- [ ] Have a name, that is unique in their parent Widget
- [ ] Can be accessed using absolute or relative paths
- [ ] The outside can request only Emitters (Property Emitters and OPlugs) from the Widget
- [ ] Have a PropertyReader handle for Property Operations

## Input Plug
- [X] Concrete implementation of the Receiver interface.
- [X] Store a constant raw reference to their owner Widget that is always valid.
- [X] Optionally forwards ValueSignals to a Widget.InputCallback, passing a Widget.Handle handle to the owner Widget.
- [X] Which method is forwarded to can be changed by the owner Widget.
- [X] If the Callback throws an exception, the ValueSignal will remain unmodified. 

## Output Plug
- [X] Concrete implementation of the Emitter interface.
- [X] Have no knowledge of the Widget that owns it.
- [X] Can emit a Value, either from within or from the outside of their owner Widget.

## Property
- [X] Have an associated Widget
- [X] Store a single Value, whose Schema is constant
- [X] Property Values cannot be None
- [X] Have an optional, constant Operations that is applied to each new value
- [X] Property.Operations have a private data Value
- [X] Property.Operations have access to a Widget.View of the Widget owning the Property
- [X] Never accept or block Signals
- [X] Have an optional, mutable Property.Callback that is invoked after each Value change
- [X] PropertyCallbacks have access to a Widget.Handle handle of the Widget owning the Property
- [X] PropertyCallbacks must guard against infinite cycles

# The Widget

A Widget's parent is constant and is guaranteed to be valid at all times. That means, you can not re-parent a Widget in the Scene as this would change its path. You can however change its position among its siblings, see "The Z-Value", which does not influence its path at all.

> **When Widgets were Nodes**  
> Originally, the Scene was designed to contain abstract "Nodes", instead of Widgets. A Node could be anything that lives within the Scene hierarchy and that would live and die with its parent Node. I assumed that Services, Resources, Layouts and Controller objects would all share the same hierarchy.
> However, one by one, the other use-cases for Nodes disappeared. Resources are better managed somewhere outside the hierarchy, where they can pre-fetched, cached and replaced without having to worry about their place in the Scene. Controller objects were replaced with the single Circuit, Services are now globally accessible from the Application. And Layouts, the only object that closely interacts with Widgets, has been incorporated into the Widget class itself (see below).
>
> When it came time to design the `Node.create_child` method, and I wanted to pass in a Node subclass as argument, I realized that this would not work with a purely data-driven approach, in which a Node can be fully realized through user-defined data (Widget.Definition, Code, etc.) alone. Instead, you would need access to built-in types, maybe through string identifiers ... which would work but isn't great. Also, so far I have only come up with a single class anyway: the Widget.
>
> Therefore, the "Node" has now subsumed the "Widget", which was planned as subclass but is now the one and only class that is stored in a Scene hierarchy. This way, we save on a few dynamic casts as well.

> **Is-a-Widget VS. Has-a-Widget**  
> When Widgets and Nodes were different concepts, I thought about turning could turn the Widget -> Node relationship from an "is-a" to a "has-a" relationship. Meaning instead Widget deriving from Node, we could simply contain a Node as a field in Widget. 
>
>Usually this is preferable, if not for one thing: discoverability. What I mean is that you can find a Node using its name in a Scene, but if the Node knows nothing about Widgets (or potential other Node-subclasses), then you don't get to a Widget from a Node. Instead, when a Widget *is-a* Node, the Node class can still be kept in the dark about the existence of a Widget class, but you will be able to `dynamic_cast` from the Node to a Widget.
>
> Now that there is no difference anymore, this discussion is mute of course, but still an interesting though in its historic context.

## Widget Type

Widget Types are objects that describe the Widget, much like a Schema describes a Value. You can create new Widget Types at runtime, by constructing a Widget.Definition object, populating its fields and registering it with the Scene. Every Widget.Type has a name that is unique in the Scene. It can be used to construct a new Widget of that type and Widget instances return it as their type identifier.

## Output Plug

Output Plugs (OPlugs) are Emitters that live on the Widget and than can be invoked to propagate non-Property Values into the Circuit as needed. They share a namespace with the rest of the Circuit Elements that are discoverable on a Widget (IPlugs and Properties).

Unlike IPlugs, OPlugs are public -- meaning that they can be triggered from everywhere, although most will probably be triggered from code living on the Widget itself. The reason why we allow them to be triggered from outside of the Widget is that they themselves do not depend on the Widget itself, nor do they modify it in any way. It is therefore feasible to have for example a single Node with an OPlug "FormChanged" which is triggered from the various form elements as needed.

## The Z-Value

Widgets can and do overlap a lot on the screen. Children always overlap their parent Widgets but (depending on the parent's Layout) can also overlap with siblings. The order in which Widgets are drawn on screen and the order in which they receive "top-down" events like mouse clicks is determined by their "Z-Value", a single floating point number that identifies a Widget's "depth" among all other Widgets. Unlike the sibling order, which is only relevant to the parent, the Z Value is global, meaning two Widgets can compare their values to determine their relative placement, no matter where in the hierarchy they are.

The root Widget has the Z-Value zero by definition, all other Z-values can be calculated by the following (recursive) Python snippet:

```python
def get_z_value(node):
    # child widgets are drawn on top of (with a higher z-value than) their parents
    parent: Optional[Node] = node.get_parent()
    if parent is None:
        return 0  # node is the root
    parent_z_value: float = parent.get_z_value()
    
    # the available space for child widgets is the difference between the parent's and the next uncle's z-value
    grandparent: Optional[Node] = parent.get_parent()
    if grandparent is None:
        available_space: float = 1
    else:
        available_space: float = 1 / (grandparent.get_child_count() + 1)
    
    # the z-value of a child widget is determined by its position within its siblings and the number of siblings
    # the lowest index is a 1, otherwise we'd have a z-value of zero for all nodes on the left edge of the tree
    # we use sibling count plus 1, otherwise nodes on the right edge of the tree could have a z-value greater than 1
    local_z_value: float = (parent.get_child_index(node) + 1) / (parent.get_child_count() + 1)
    return parent_z_value + (local_z_value * available_space) 
```

Whenever a Widget changes the order of its children, the Z-values of all affected child Widgets must be recalculated. I would think that it might be better to just do it right away, since it should not happen very often (every second maybe, instead of every millisecond) and the calculation itself is not very complicated. It's just a lot of jumping around in memory. Maybe we could fix that by passing in all required values up-front (basically everything that needs to be "gotten" in the snippet above) and do a breadth-first traversal of the descendants of the Widget that changed its child order? Whenever you hit upon a child whose z-value did not change, the traversal of that sub-tree can be cut short.
What's going to be more of a problem (expense-wise) is the introduction or removal of child Widgets, since that means recalculating all z-values of all descendants... Maybe the z-value is something that we can calculate on-the-fly as needed?
Jup .. that sounds like a good idea - calculate it on the fly every time you need it. You don't even need it to iterate in order as that is completely different.


## Properties

Widget Properties are Values associated with a Widget instance. They are determined by the Widget.Definition and are basically member fields. Every Property has a name that is unique in its Node.Definition and also unique among Input and Output Plugs. Properties must be Values, which has the advantage that their content can be emitted and received through the Circuit and that they can be serialized automatically. Properties have an associated Value Schema that cannot be changed. Widgets store no data other than what is stored in their Properties (see "Private Data or not?" for a discussion of an alternative). If you take the entire Scene, take all Properties of all the Nodes, serialize the Scene alongside the Properties and re-load them, you have essentially restored the entire UI application at the point of serialization. Properties cannot store empty Values.

### Property Operation

Apart from their Value Schema and name, a Property is defined by an optional Operation that is performed on every new Value. Like in an Operator, the Operation can either choose to return the input Value unmodified, change it in some way (clamp it, round it to the nearest integer ...) before returning it or returning None, in which case the existing value of the Property remains unchanged. Unlike Operator.Operations, Property.Operations have access to a Widget.View object. This way, Operation are able to read other Properties of the same Widget which allows for a Property which is a number that is clamped between two others, for example.

Property Operations cannot be changed as they are as much part of the Property "type" as its Value Schema.

Like Operator Operations, Property Operations are allowed to store an arbitrary Value as private data. This actually fixes the problem I had with "private" Properties that were used only to act as support data for other Properties (See "Private Data or not?").

### Property Callback

Like Input Plugs, Properties also have an optional Callback function that is invoked after the Property has been updated. Unlike the Property Operation, which is only allowed to operate on the value, the Callback has full access to a Widget.Handle handle, which allows it to update Properties and emit from Output Plugs. Of course, you have to take care not to create any infinite loops between Callbacks, just like you would in the Circuit. If a Property Callback of a Property would set the same Property again, it would create an infinite loop and an immediate deadlock if it was not for the reentrancy check in a Property's `Emitter._emit` methods that will catch this case and fail gracefully. The same goes for the "wider" cycle, in which two Properties keep changing each other's value forever.

However, there is one exception to the "no cycle" rule, one that is only possible because of the separation between the Property Operation and -Callback: Imagine a Widget with two Properties `A` and `B` that need to be kept in sync. Let's say that `B=A+2` must hold at all times. Whenever you set `A` to `x`, its Callback will also set `B` to `x+2`. But there is no way to differentiate this case from one where you changed `B` instead of `A` and so `B` will continue to update `A` as well ...which would cause a NoDag exception. But while this is a cycle, it is clearly not an infinite one. In fact, after touching both `A` and `B` once (no matter which one is set by the user), no further progress will be made. We can catch this case with a simple all-purpose optimization: check whether the new value is equal to the existing one *prior* to checking for cycles. If the value is the same, just do nothing and return. In this case, setting `A`  to `x` will change `B` to `x+2` which will in turn cause `A` to update to `x` again. But since at this point, `A` already has the value `x`, it will simply return nothing and the cycle is broken.

Like IPlug Callbacks, Property Callbacks can be changed by the Widget and set to None to disable.

### Properties and ValueSignals

Properties can be updated both from within a Widget Callback and from an Emitter upstream in the Circuit. Only in the second case will the Property even see a ValueSignal.  
Here are all the ways that we could do this:

1. Have the Property Operator handle ValueSignals
   Not great, that's not what it is for. Also, I don't want to create a bogus ValueSignal every time I update a Property.
2. Have the Property Callback handle the ValueSignal
   Better, but we still need a bogus Signal ... and worse yet, I cannot think of a Property that would even accept or even block a Signal. It's about the data of the Signal.
3. Ignore existing Signals and don't create one if you set the Property through code.
   Sounds reasonable. 

Nr. 3 it is.

### Property Visibility

In previous versions, Properties had a flag associated with their type called "visibility". A visible Property was one that, when changed, would update the visual representation of the Widget and would cause a redraw of its Design.
I chose to get rid of this in favor of a more manual approach, where you have to call "redraw" in a Widget manually, because whether or not the Design changes is not a 1:1 correspondence on a Property update. Some Properties, for example, might only affect the Design if they change a certain amount, or if they are an even number... or whatever.

### Private Data or not?

In the beginning, it was planned that all data stored in a Widget would have to be a Property. This is similar to the Python approach of "public everything", where you can access all of an objects members from the outside with only a naming convention as guide (members starting with "_" are private).  
Then we came across a use-case that saw a Property that would only change if the new value was sufficiently different from some sort of "rest" value. In that model, we would have a second Property that would only serve as a support value for the first one. This second Property will not connect to anything else in the Circuit, leaving about 72 bytes (with the current implementation) unused. That is per unused Property per instance of that Widget type.

The alternative would be to store a single Value in the Widget that contains private data that is not accessible from the outside. It would also be serialized with the Widget, just like a Property, but without any of the Emitter/Receiver overhead. With this approach it is left to the programmer to decide, what values are exposed on the outside of the Widget and what are private.

As always, I think it comes down to a judgement call. Let's say that we have 40 Properties on a Widget, 16 of which are private. The complete UI consist only of Widgets of that type (or similar) and we end up with about 600 Widgets on screen. I think both of these numbers might be a bit high, but let's go with that.
With the current implementation, we have 16 * 600 * 72 = 691200 bytes = 675Kib [(Kibibyte)](https://en.wikipedia.org/wiki/Kibibyte) = not so much, for a ridiculously overcrowded UI.

Then again, it is not nothing. And 600 Widgets might actually be quite reasonable if you count Widgets that are invisible, maybe cropped, maybe hidden behind others. And having private data would mean that the user is better able to keep the public interface of a Widget clean from implementation details. This sounds especially relevant, when you start versioning Widget Definitions.

Okay, let's say have have this private Data Value. It will most likely be a map. Does the map share the namespace of its keys with the namespace of the Widgets I/OPLugs and Properties?  
No. In order to access the data, you need a Widget.Handle handle that has a special method to access the private data, maybe called `data(name)`.  To make this work consistently, we could enforce that the data is indeed always a map. With this special method, you don't need to worry about accessing anything but the private data, so there is no need to share the namespace. 

That "stable interface" argument almost had me there, but you could also simply put an underscore in front of the name to denote it as "private" (like in Python). While it is true that some user of that Widget might still depend on a private Property, he does so with the implied understanding that this Property may be renamed, or removed in the future.  
What I am more worried about is that users that write code for a Widget will constantly have to think about whether this one value is stored in a Property or if it is stored in the private data. I certainly would, especially since you can have both a Property named "Foo" and a private data Value of the same name. I think this would lead me to favor one over the other and since I am always for private-first, I would probably end up using mostly private data and only occasionally public properties whenever I see an obvious use-case.  
 The problem with this approach is that I explicitly set out to "fix" the problem I saw in Qt, where certain values are not accessible from the outside even though I am sure are stored somewhere in the Widget (I cannot think of the example from the top of my head now though). By making everything public by default, we can be certain that the implementer of a Widget will not hide important information from the outside -- may that be by accident or by making everything private-first because that's what you do in OOP.

Another aspect and another reason *for* forcing all data to live in Properties is that with that approach you can actually get the best of both worlds: just have a map Value Property called "_data" that stores all of the private data. This way, it is not hidden from the outside but it is stored in a format that makes it appropriately difficult to get any meaningful content out of it. And it is safe to do so, because even though you can *read* all of the "private" Properties on a Widget, you cannot *write* to any of them.

# notf vs. Redux

The first of [the three principles of Redux](https://redux.js.org/introduction/three-principles) is the idea that the entire application state is a single object. This, combined with an immutable object model allows for easy implementation of complete application serialization / undo-redo / copy / diffing etc. (as seen in the quite brilliant talk [Postmodern immutable data structures](https://www.youtube.com/watch?v=sPhpelUfu8Q)). And we _do_ use immutability extensively throughout the notf architecture. However, our application state is not a single object but rather a wild collection of Widgets and free Circuit Elements.  
How can we live with ourselves in 2020, where the world has seemingly moved on from this entangled web of interconnected objects into the  promised land of the unidirectional data flow? 

Well, we know the pros, but what about the cons? Let us imagine how our application state would work in a single value:

* Instead of pointers, we now have to do a look-up every time one object references another one. 
* References do not influence ownership anymore and in order to clean up dead objects you have to do your own garbage collection.
* The undo/redo "pro" argument doesn't actually apply, because undoing a change in the UI alone is pointless, if the underlying model is not also affected.
* We have a potential large application state and a lot of small changes. Right now, these changes are local to individual Values only, if we always create a new application state for every Value change, we will end up creating a lot of unnecessary work.
* We can already serialize and deserialize the entire application - the fact that the serialized representation looks different from the runtime representation is not a bug, it's actually a feature.

On the actual pro side: diffing a complete application would be very nice to do if we ever go ahead and do multithreaded event handling. However, that itself would add even more overhead to a feature that I had already dismissed because of overhead concerns.

So no, for now we are not storing the application into a single object.  
Maybe my view on this will change in the future, after we have a working alpha version? I expect that the effects of some design decisions will only become apparent then and it will be interesting to see how they stack up against the current hotness in web development.

---


In order to manage a Circuit, the Scene keeps a hierarchy of Nodes. Each Node is statically defined by a Node type, much like a Value is defined by its Schema. The contents of the Value might change, but the Schema stays the same. For Nodes, this means that the data that makes up the Node is mutable, but its core makeup is not.  
It consists of:

1. Properties
2. Input Plugs
3. Output Plugs
4. A dict of named Receiver Callbacks
5. A dict of named State Enter/Optional[Exit] Callback pairs
6. A set of (named State, named State) pairs that define allowed State Transitions
7. The name of the entry state
8. ~The Schema for a private data Value~
9. A child container type (?)

**Properties**  
Properties are a single Value that determines some aspect of the Node that might be of interest to the rest of the Circuit. They are both Receivers and Emitters, meaning they can change automatically based on Values upstream from the Circuit and they can influence downstream Elements in the Circuit as well.

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
4. Handle, in order to create or remove child nodes.

That looks like a function signature (self, ValueSignal) to me. A Callback has a name, which can be registered with the IPlug. An IPlug can only call a single or no Callback - we don't allow multiple connected Callbacks. Of course, you can set up the one Callback to call two other ones, but that's your responsibility.

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



# Widgets and Logic

We have now established that Logic elements (Emitter and Receivers) must be protected from removal while an update is being handled. The update process is also protected from the unexpected addition of elements, but that does not matter for the following question, which is about the relationship between logic elements and Widgets.

First off, Widgets own their logic elements. This is elementary and one of the features of notf: a Widget is fully defined by the combined state of its Properties, while Signals and Slots are tied to the type of Widget like methods are to an C++ object. But do some logic elements also own their Widget? Properties should not, they are basically dumb values that exist in the context of a Widget but are fully unaware of it. Signals do not, they are outward facing only and do not carry any additional state beyond that of what any Emitter has. Slots however can (and often will) require a strong reference to the Widget that they live on. What now if we face problem 1 from the chapter "Logic Modifications" and Widget B removes Widget C and vice-versa? 

It is clear that an immediate removal is out of the question (see above). We still need to remove them, but only when it is absolutely safe to do so. And the only time during an update process when it is safe to do so is at the very end.
At the moment, the Widget is flagged for deletion, a strong reference to it is added to a set of "to delete" Widgets in the Graph. Then, at the very end of the update, we discard every Widget that has an ancestor in the "to delete" set and delete the rest. This way, we keep the re-layouting to a minimum. 

Note that it is not possible to check whether a Widget is going to be removed at the end of the update process or not, even though such a method would be easy enough to implement. The problem is the same as with an immediate removal. 
Depending on the execution order, the flag of Widget A would either say "this Widget is going to be deleted" if B was updated first or "this Widget is not scheduled for removal" if A is updated first. The result is not wrong, but essentially useless.
The same goes for anything else that might be used as "removal light", like removing the Widget as child of the parent so it does not get found anymore when the hierarchy is traversed.


