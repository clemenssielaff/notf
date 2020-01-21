# The Scene

The Scene takes its name from the "Scene Graph" that is inherit in many game engines or DCC tools. It is older than the Circuit in its conception and is a higher-level abstraction to the workings of a UI. The Scene is conceptually somewhat orthogonal to the Logic, but the Scene module depends on the Logic module and not the other way around.

Data-structure wise, the Scene is a tree of Widgets. There is one RootWidget in a Scene, and like every other Widget, it can have 0-n children. Every Widget has a single parent that outlives it and cannot be changed. When the Scene is removed, it will safely destroy all  Widgets from the leafs up, meaning that the root is the last Widget to be destroyed.

A Scene owns a Circuit and manages it in a way that allows the user to inject user-defined code (Scripts) at very specific places that are as expressive as possible while still being safe. All modification of a Scene (and its Circuit) is explicitly single threaded and should occur on a dedicated thread. You can have parallel loops or similar technicalities where they make sense, but they must operate on local data only. Much like Circuit Elements, the removal of a Widget in the Scene is delayed until the end of an Event.

# Quick Facts

## Scene
- [ ] Owns the root of the Widget hierarchy and safely deletes it up on destruction.
- [ ] Owns the Circuit that contains all Widgets' Circuit Elements (Input/Output Plugs and Properties).
- [ ] Keeps a queue of Widget.Handles to delete after an event has finished.
- [ ] Can create first-level Widgets that are direct children of the root.
- [ ] Can safely remove all expired Widgets, with children deleted before their parents.
- [ ] Can find and return Widgets and Widget Elements using absolute paths
- [ ] Can create Widget.Types from Widget.Definitions

## Widget
- [ ] Are constructed from a Widget.Type object
- [ ] Have Input and Output Plugs
- [ ] Have Properties
- [ ] Have a state machine
- [ ] Have a single Value containing all Properties
- [ ] Have a dict of named Callbacks
- [ ] Have a single, constant and always valid parent Widget (except the Root)
- [ ] Have 0-n children stored in a dict by name.
- [ ] Have a single Layout that can produce a strong order of the Widget's children
- [ ] Have a name, that is unique in their parent Widget
- [ ] Has two Handles: one "view" that cannot mutate the widget and a "handle" that can
- [ ] Have a Z-value, which is globally strongly sortable.

## Input Plug
- [ ] Concrete implementation of the Receiver interface.
- [ ] Store a constant raw reference to their owner Widget that is always valid.
- [ ] Optionally forwards ValueSignals to a Widget.InputCallback, passing a Widget.Handle handle to the owner Widget.
- [ ] Which method is forwarded to can be changed by the owner Widget.
- [ ] If the Callback throws an exception, the ValueSignal will remain unmodified. 

## Output Plug
- [ ] Concrete implementation of the Emitter interface.
- [ ] Have no knowledge of the Widget that owns it.
- [ ] Can emit a Value from their own Widget only

## Property
- [ ] Have an associated Widget
- [ ] Store a single Value, whose Schema is constant
- [ ] Property Values cannot be None
- [ ] Have an optional, constant Operations that is applied to each new value
- [ ] Property.Operations have a private data Value
- [ ] Property.Operations have access to a Widget.View of the Widget owning the Property
- [ ] Never accept or block Signals
- [ ] Have an optional, mutable Property.Callback that is invoked after each Value change
- [ ] PropertyCallbacks have access to a Widget.Handle handle of the Widget owning the Property
- [ ] PropertyCallbacks must guard against infinite cycles


# The Widget

A Widget is the basic building block of of the Scene's tree structure. It is an architectural detail and not accessible to user scripts. Every Widget has a single parent, which is constant and guaranteed to be valid throughout the Widget's lifetime -- except for the root Widget, whose parent is always empty. The root Widget is created with the Scene and is the last Widget to be destroyed.
  
Every Widget has 0-n child Widget in no particular order. New Widget can only be created by their parent Widget, and only be removed by their parent (except the root, which is created and destroyed by the Scene). When a parent Widget removes one if its children, the child will remove all of it children first before destroying itself.  
It is not possible to change a Widget's parent as that would change the Widget's Path. You can however change its position among its siblings since that operation does not influence how the Widget would be addressed in the Scene. For more details see Widget.Path and Z-value below.

> **What is a Widget**
> Originally, everything was a Node. I assumed that Widgets, Services, Resources, Layouts and Controller objects would all be subclasses of Node and share the same hierarchy. I also assumed that the user would be able to derive from the Node class to define custom Node types.  
> However, one by one, the other use-cases for most Node classes disappeared. Resources are better managed somewhere outside the hierarchy, where they can pre-fetched, cached and replaced without having to worry about their place in the Scene. Controller objects were replaced with the logic Circuit, Services are now globally accessible from the Application. And Layouts became a field inside a Widget, not a Node itself. At that point, all Nodes were Widgets.


## Widget Type

Up until quite recently in the design process, I assumed that the user would be able to subclass the Widget base class to define custom Widget types. This is a natural thing to do in C++ or any object oriented (OO) programming language, but for notf it posed some unique challenges. First of all, defining a subclass means that the user would be allowed to define custom members and modify them during the class' virtual methods -- all user Scripts were potentially stateful. Furthermore, in order to create child Widgets of a custom type you need to pass the subclass as an argument in an interpreted language which means that you required access to the dynamic type objects and the interpreted context in which they live.  
Both things did not sit well with the purely data-driven approach that I was going for.

Fortunately, I discovered that it was possible to contain all user-define code into `Callback` objects, basically strings. At least for an interpreted runtime. Statically compiled Callbacks would compile with the rest of the program, but I figured that the true power of notf is first and foremost in its live updates, which require (?) a runtime. If you indexed each Callback by a name, you basically broke down the whole "subtype" into a map of string to string. Combined with other POD (Value Schemas and the State Machine, see below), this lead to the invention of the Widget.Type object. Widget.Types are objects that describe the Widget, much like a Schema describes a Value. The contents of the Value might change, but the Schema stays the same.  
You can create new Widget Types at runtime, by constructing a Widget.Definition object, populating its fields and registering it with the Scene. Every Widget.Type has a name that is unique in the Scene. It can be used to construct a new Widget of that type and Widget instances return it as their type identifier. The only difference between the Definition and the Type is that the Type is validated and is immutable.
  
A Widget.Definition object consists of:

* The Widget.Type's *type_name* (str).
  This is the Scene-unique name of the Widget type. The name cannot be empty and must not contain Path control symbols. The type name also acts as the default name for a new Widget instance.
* All *Input Plugs* and *Output Plugs* (Dict[str, Value.Schema])
  Unlike Properties, Plugs do not hold a Value, they just receive or emit one. They are defined by a Value Schema only.
  * All of the Widget's *Properties* (Dict[str, PropertyDefinition]).
  Properties need to be defined themselves, albeit just with a Schema and an optional Property.Operation. See below for details.  
  All Properties and input/output Plugs share the same namespace. Their names cannot be empty and must not contain Path control symbols.
* Free *Input Callbacks* (Dict[str, InputPlug.Callback])
  Callbacks that can be attached to an input plug
* Free *Property Callbacks* (Dict[str, Property.Callback])
  Callbacks that can be attached to a property
* The Widget's *StateMachine* (StateMachine)
  The StateMachine controls which Callbacks are called when and provides the initial set-up Callback for the Widget type.


## Widget Paths

Unlike Circuit Elements, which have a Circuit-unique numeric identifier, Widgets are uniquely identified and addressable using their "Path". The Path of a Widget is simply the concatenation of all ancestor Widgets' names, separated by a reserved delimiter symbol. Prerequisite for this is of course, that all Widgets have a name that is unique within the parent Widget. Widget names are not allowed to contain Path control symbols. The consist of:

* `.` denotes the furthest Widget in the Path up to that point or "this" Widget, if it is the first name. `..` means "go up one level".
* `/` is the name separator. The name to the left is the parent of the name to the right if none of them are control symbols.
* `:` is the separator between the Widget Path and the target Element. See sub-chapter for details.

By having the identifier of a Widget be another POD object (in this case a string), user Scripts are free to address Widgets all over the Scene. Paths in user Scripts can be absolute or relative. Relative Paths are resolved by replacing the initial dot with the absolute path of the Widget executing the Script. Much like filesystem paths in Linux, you can use the reserved `..` name to go up a level in the Scene.

Example of a valid Widget Path:

* `/absolute/path/to/a/widget`
* `./child1/../child2/grandchild38` (if called from `/parent` resolves to `/parent/child2/grandchild`)

==> CONTINUE HERE

### Element Paths

IPlugs, OPlugs and Properties share the same namespace, each can be addressed as part of the Widget using the syntax "/first_ancestor/.../widget:property" (or :iplug, :oplug)


Additionally, Paths can address a single Circuit Element (Input Plug / Output Plug or Property) on the Widget. I though of splitting Paths that lead to a Widget and Paths that lead to a Property (and I/O Plugs) into different types but decided against it ultimately. If there is a single Path type for all Circuits, you don't have to specify (for example) if you want to connect to an Input Plug or Property since both share the same namespace.

This still leaves the question of whether we need two different Path types, Widget Path and Element Path? ... yeah maybe.


## Widgets and User-Code

Widgets are *the* place to build the UI through user-defined code. Of course, the code cannot be just injected anywhere, we have a specific place for it: Callbacks. A Callback is something that can be connected to an Input Plug (IPlug), the connection can be modified at run-time. This Callback receives a Value from the IPlug, but in order to work effectively, it also needs access to other members of the Widget - some of which might have been user-defined as well. What are these things?

1. Properties
2. Output Plugs
3. Existing child Widgets
4. Handle, in order to create or remove child Widgets.

That looks like a function signature (self, ValueSignal) to me. A Callback has a name, which can be registered with the IPlug. An IPlug can only call a single or no Callback - we don't allow multiple connected Callbacks. Of course, you can set up the one Callback to call two other ones, but that's your responsibility.

A pure function only operates on a given set of inputs, must return a value, and does not produce any side effects.

Unlike in previous architectures, you never get a handle to another Widget. I always assumed that there would be some way to request a handle to another Widget from the Scene, something like:

```python
this: Widget.Self = self
grandchild: Optional[Widget.Handle] = this.find_widget("./child/grandchild")
assert grandchild is not None
grandchild.do_something(...)
```

However, what is this `do_something` that we need to call on another widget? Let's enumerate all use-cases that I can think of off the top of my head:

1. Get a Property Value
2. Connect / Disconnect an input of this Widget to the output (Plug / Property) of another one.
3. Connect / Disconnect an output of this Widget to an input of another one.
... are there even any other?

But what about `Widget.get_parent()`. I mean, the Widget needs it in order to do the layouting, but is that something that the user needs to see? If you want to connect to a parent Property or whatever, you do that via the Path. Easy.

Point is, the user never has access to the widget itself, only through various handle classes that are passed into Callbacks. 

## Input Plug

Widgets have input and output plugs, special Receiver and Emitters that that allow the Widget to communicate with the rest of the Circuit. Input Plugs (IPlugs) take a ValueSignal and ignore all others (apart from disconnecting, which a Receiver does automatically). They then forward the ValueSignal to a Callback inside the Widget. Which callback they forward to can be changed at runtime as part of the Widget's StateMachine.

## Output Plug

Output Plugs (OPlugs) are Emitters that live on the Widget and than can be invoked by their Widget to propagate non-Property Values into the Circuit as needed. They share a namespace with the rest of the Circuit Elements that are discoverable on a Widget (IPlugs and Properties).

> Previously output plugs were allowed to emit from anywhere with the reasoning:
>> Unlike IPlugs, OPlugs are public -- meaning that they can be triggered from everywhere, although most will probably be triggered from code living on the Widget itself. The reason why we allow them to be triggered from outside of the Widget is that they themselves do not depend on the Widget itself, nor do they modify it in any way. It is therefore feasible to have for example a single Widget with an OPlug "FormChanged" which is triggered from the various form elements as needed.
> Now however, I've decided to limit emission ability to the Widget owning the output plug, since in the reasoning above, the child widget would have to have explicit knowledge of their parent. I would rather invert this, with only the parent knowing of its children but not the other way around. That means that each child is free to emit from its own output, the parent is able to forwards the child's output to their own and this way encapsulation is preserved. 

## Properties

Widget Properties are Values associated with a Widget instance. They are determined by the Widget.Definition and are basically member fields. Every Property has a name that is unique in its Widget.Definition and also unique among Input and Output Plugs. Properties must be Values, which has the advantage that their content can be emitted and received through the Circuit and that they can be serialized automatically. Properties have an associated Value Schema that cannot be changed. Properties cannot store empty Values.

Widgets store all of their Properties in a single Value and have no data other than what is stored in their Properties (see "Private Data or not?" for a discussion of an alternative). If you take the entire Scene, take the combined Property Values of all the Widgets, serialize them alongside the hierarchy and re-load it, you have essentially restored the entire UI application at the point of serialization. 

Properties are a single Value that determines some aspect of the Widget that might be of interest to the rest of the Circuit. They are both Receivers and Emitters, meaning they can change automatically based on Values upstream from the Circuit and they can influence downstream Elements in the Circuit as well.

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


## Widget State Machine

### State Switching

The question right now is if Receivers always call their new callbacks right away or not - and if they do, in which order?

My first instinct was to say "Whenever a Callback is reassigned, the new one is called immediately with the latest Value". Then I realized, that during a State change this would mean that the first Callback is called before the last Callback in the State change has a change to switch leading to weird race-conditions where one Callback somehow triggers others (through Properties) even though they are currently undergoing a State change.
The next step was to say, after a State change, all new Callbacks are called once, this way we don't have any in-transition calls. This sounds like it is required for a working system but not sufficient yet. After all, the order in which the Callbacks are called is also important. Ideally, it shouldn't but I am not sure if I can rely on orthogonality between Callbacks.

Actually, I can be sure that they are not orthogonal. What about a Widget that has different Callbacks for "mouse down" and "mouse up"? Even though it isn't specified anywhere in the Widget, it is reasonable to expect "mouse down" to be the first call and a corresponding "mouse up" before the next one.

Maybe then, this idea of re-emitting Values from IPlugs and Properties is a bad idea to begin with? 
Yes it is.
Instead, a State change should go like this:

1. StateA:Enter function is called. StateA is the entry State of the Widget.
2. Somehow a transition to StateB is triggered.
3. StateA:Exit function is called
4. StateB:Enter function is called


## The Z-Value

Widgets can and do overlap a lot on the screen. Children always overlap their parent Widgets but (depending on the parent's Layout) can also overlap with siblings. The order in which Widgets are drawn on screen and the order in which they receive "top-down" events like mouse clicks is determined by their "Z-Value", a single floating point number that identifies a Widget's "depth" among all other Widgets. Unlike the sibling order, which is only relevant to the parent, the Z Value is global, meaning two Widgets can compare their values to determine their relative placement, no matter where in the hierarchy they are.

The root Widget has the Z-Value zero by definition, all other Z-values can be calculated by the following (recursive) Python snippet:

```python
def get_z_value(widget):
    # Child widgets are drawn on top of (with a higher z-value than) their parents.
    parent: Optional[Widget] = widget.get_parent()
    if parent is None:
        return 0  # widget is the root
    parent_z_value: float = parent.get_z_value()
    
    # The available space for child widgets is the difference between the parent's and the next uncle's z-value.
    grandparent: Optional[Widget] = parent.get_parent()
    if grandparent is None:
        available_space_per_child: float = 1
    else:
        available_space_per_child: float = 1 / (grandparent.get_child_count() + 1)
    
    # The z-value of a child widget is determined by its position within its siblings and the number of siblings.
    # The lowest index is a 1, otherwise we'd have a z-value of zero for all widgets on the left edge of the tree.
    # We use sibling count plus 1, otherwise Widgets on the right edge of the tree could have a z-value greater than 1.
    local_z_value: float = (parent.get_child_index(widget) + 1) / (parent.get_child_count() + 1)
    return parent_z_value + (local_z_value * available_space_per_child) 
```

The Z-value is calculated anew every time you ask for it.  
While that might sound expensive, the alternative is worse: In order to cache the Z-value of all Widgets, it needs to be re-calculated every time a parent Widget changes the order of its children, or introduces a new child. Removing an existing child does not matter since the absolute Z-values do not matter and their order is not affected. And the calculation itself is not very complicated. It's just a lot of jumping around in memory. Maybe we could even get rid off a lot of jumps if we traversed the tree in a breadth-first fashion, which would allow us to get rid of most of the `getters` in the code above. Furthermore, you are able to cut the traversal short whenever you hit upon two siblings whose Z-values did not change...  
The problem here is not efficient tree traversal but that the tree can grow large quite quickly. Whenever you add or move a new new first-level Widget, you would end up recalculating the Z-values of all Widgets in the entire Scene. This is a problem because we need to minimize the maximum amount of work done during any event, not the absolute amount of work done across all events. The bane of UI is not throughput but latency.  

Note that the Z-value is not needed for Scene traversal, it is only required if you take two random Widgets in the Scene and want to know which one is drawn on top of the other.

# Widgets and Logic

We have now established that Logic elements (Emitter and Receivers) must be protected from removal while an update is being handled. The update process is also protected from the unexpected addition of elements, but that does not matter for the following question, which is about the relationship between logic elements and Widgets.

First off, Widgets own their logic elements. This is elementary and one of the features of notf: a Widget is fully defined by the combined state of its Properties, while Signals and Slots are tied to the type of Widget like methods are to an C++ object. But do some logic elements also own their Widget? Properties should not, they are basically dumb values that exist in the context of a Widget but are fully unaware of it. Signals do not, they are outward facing only and do not carry any additional state beyond that of what any Emitter has. Slots however can (and often will) require a strong reference to the Widget that they live on. What now if we face problem 1 from the chapter "Logic Modifications" and Widget B removes Widget C and vice-versa? 

It is clear that an immediate removal is out of the question (see above). We still need to remove them, but only when it is absolutely safe to do so. And the only time during an update process when it is safe to do so is at the very end.
At the moment, the Widget is flagged for deletion, a strong reference to it is added to a set of "to delete" Widgets in the Graph. Then, at the very end of the update, we discard every Widget that has an ancestor in the "to delete" set and delete the rest. This way, we keep the re-layouting to a minimum. 

Note that it is not possible to check whether a Widget is going to be removed at the end of the update process or not, even though such a method would be easy enough to implement. The problem is the same as with an immediate removal. 
Depending on the execution order, the flag of Widget A would either say "this Widget is going to be deleted" if B was updated first or "this Widget is not scheduled for removal" if A is updated first. The result is not wrong, but essentially useless.
The same goes for anything else that might be used as "removal light", like removing the Widget as child of the parent so it does not get found anymore when the hierarchy is traversed.

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

----

# Widget Rendering Frontend

> Apparently fighting games have a medium response time of about 67ms (which would be 4 frames on 60hz)
> https://en.wikipedia.org/wiki/Input_lag#Typical_overall_response_times

* Design is a Node underneath a Widget, this way a Widget can have multiple Designs. This way we can switch easily or separate constant from changing parts of the complete design.
* Designs can be disabled. This way you can switch between multiple designs or temporarily hide a Widget that will reappear soon. 
* Use a skip list as data structure for a root design. Disabling a design will mark it to be skipped, so does deleting one.
* It would be *very* advantageous to forbid designs to change size at run-time. This way, we could pack them tightly into the skip list and wouldn't have to worry about resizing the list during design update. You could also update all designs in parallel and write them straight into the final design buffer because you know that they won't grow (I guess shrinking is okay). 
* Maybe if we really cannot enforce a size limit on a design, we could at least separate those that can give that guarantee from those that cannot.

Design and how to "View" a Widget.

Ideally, the Design would not be a part of the Widget at all. Instead, the Design would be a view onto the Widget.  
That leads to the following questions:

1. Where is the function that transforms a Widget into whatever representation?
2. Should a Widget have a single Design object, or should it have multiple? Which one is active? Does the Design change or just the transformation function?

Answers:

1. Previously, it was part of the Widget class. Which makes sense because it requires full knowledge of the Widget and is tied to the Widget's type.
I guess, the Widget type can just have an additional field "renderers": Map[str, RenderCallback], with each RenderCallback taking a Widget.View and outputting a Design. 
But we need different Designs for different renderers...
2. I liked the idea of having multiple design objects per Widget, so you can easily switch between the two. However, just for the standard case of a 2D UI, the idea of a Design "skiplist" with sub-Designs of fixed size, this will not give us much. I guess the cost of replacing a sub design is about the same as updating it. True, you can have multiple sub-designs in the same Design and en-able or dis-able them, but I doubt that we will have many Widgets with multiple, static Designs that are just toggled in between. And even if. By not having both sub-designs part of the same Design but only using one of them, we shrink the top-level design and maybe that will offset all gains by having the multiple sub-designs in there..?
So I guess we are back again at just one design per Widget. I _do_ like the idea of fixed Designs though, especially since they would allow us to update the complete Design with multiple threads.

## Where to place the Design

at the very beginning, a Design did not exist until the "Painterpreter" went through the tree and collected all the sub-designs from all Widgets. This is viable but no optimal. What we want instead is a Design that is basically a value and each redraw of the scene performs a minimal update of that value.
So, where do we place that value?

How about placing it into the tree as a Node? All child Nodes would influence the Design and if you delete the Design you also delete all the nodes. Hm..

What if we lift it one level higher? There is a tree of Designs, every Design has an explicit root Node that it represents and child Designs...
Now you suddenly don't have a single root Node that you can hang Widgets onto - you have [1-n] Designs (or Canvases or Document or whatever) and each has its own root.

Well, how are these things different? Actually, when you place the Designs right into the tree, you can easily have a Widget contain a complete Design. If you place the Design hierarchy outside of the Widget hierarchy, that will become harder to do. Also event handling etc. 

Okay - so we need Designs and Subdesigns _inside_ the Node hierarchy. 

That still leaves the problem of updates and consolidation. 
1. How does a Widget that is updated let its Design know that it has changed? 
2. How does a Design let its parent Design know that is has changed? 
3. How does the root Design collect all child Designs to draw itself on screen?
Actually, 1 and 2 are basically the same - just the reporting Node is a Widget is one and a Design in the other.

I guess it all comes down to if and how we allow Nodes to be moved in the hierarchy, as unrelated as that might sound at first. The only two possibilities that I see are either:
1. re-discover the relevant nodes every time you need them
2. store them and update when necessary

My first instinct was of course to store them, but there is a problem attached to that, at least for nodes lower in the hierarchy that look for nodes higher up. If we have a Hierarchy

```
    A
  ┌─┴─┐
  B   C
┌─┴─┐
D   E
```

and want to move `B` underneath `C`, then `B` and all of its children (here only `D` and `E`) would have to update their Design. True, afterwards, they would have direct access to the Design (until it changes again) but we are designing a system for responsiveness here. It will probably still be better to re-discover the Design every time (which will be in the order of 10s of steps, not like a 1000 I suppose) than to pay a high one-time cost which might result in a stutter.

Actually: here's another idea: What if a Node (Widget or Design) simply reports itself as being "dirty" into a set stored in the Scene. That has constant cost and does not touch the hierarchy at all. It is then the Painterpreter's job to take that set of dirty nodes and figure out how to do a minimal update of the Design.  ... that's actually a lot better.

---


## Accessibility

widget is the same, but render function is different
meaning that the properties of a widget should be sufficient to render both into a Design and an Accessibility DOM (or whatever).
occlusion is a separate thing for visual and accessibility: an entry in a list may not be drawn because the user has scrolled away from it, but it should still appear in the accessibility dom

also:
**Chrome Accessibility Fundamentals**
https://www.youtube.com/watch?v=z8xUCzToff8

https://www.w3.org/TR/wai-aria-practices-1.1/#intro

Here's another angle on rendering: What about invisible Widgets? Obscured, scissored, zero opacity or manually hidden ... there are many ways that a Widget is part of the tree but should not be part of the rendered Document. 
Of course, I have already thought of that, Widgets in previous versions had special built-in fields for that (in addition to their properties).
But if you have multiple views on a Widget, how can you ever say that a Widget is hidden? And zero opacity doesn't even make sense for a screen reader.

Then again, what does make sense in a universal kind-of-way? In the end, you have Widgets for the stated use-case of having a visual representation of data. If you don't have a visual representation (like for a screen-reader, for example) then why would you have a Widget in the first place?
I guess I am still not having a good idea of how a non-visual interface would work. For now though, I will simply ignore it, as brash as that might sound. The reasoning here is that we are building a _visual_ representation of data and then to say "how can we make it work non-visually as well" is the wrong approach. At some point it might be easier to have a completely new UI for non-visual users, should that become a problem.

> I have since come to the conclusion that UI is just one transformation applied to the Scene - the main one, probably, but only one. Widgets must take care to contain enough data to be represented in multiple ways (accessibility)

## Layout Properties

When we defined a Widget, everything was pretty clear. You have Properties, Inputs and Outputs and various Callbacks. However, when we got to layouting, this clean taxonomy suddenly didn't suffice. With the grant size, the scene aabr (in global space) and child aabr (in local space) we have 3 "things" that have the following behavior and constraints:

1. They can be read by user code, but never set
2. They are visible from the outside and emit whenever new Values are set
3. The user should be able to attach internal Callback to it

To me, this very much looks like a Property _except_ for the fact that the user cannot set it and you cannot connect upstream emitters to them. 
Well, speaking of which: having _each and every_ Property settable from the outside is an invitation to screw with the system. So far, I haven't restricted any connection because I wanted to go the Python route and allow the user to change everything all the time. If you want to shoot yourself in the foot, you can do so, maybe you actually find a good reason to do so...
So actually, can we just leave it at that? Saying that if the user wants to set any of these Values by hand, he is free to do so but it will most likely break the layouting? ... That's one way to do it.
Another way would be to add a flag to a Property that determines if a Property can be set from the outside and if it can be set from the inside. In our case, both flags would disallow changes, which would be useless for the user but okay for us because we are not going through user defined code.
The overhead is minimal: 2 bits on the Property Type in space and a lookup whenever

* user defined code tries to connect a Property as downstream (note, no further checks are done once this succeeds)
* a property value is set from user defined code

Honestly, that sounds quite reasonable to me.
The only problem I have is the question, whether or not this is something that we want to include into our design. Let's make a pro/con list, shall we?

Pro: it's a natural fit for what I am trying to do with layout properties
Con: it's an additional thing that people have to be aware of when coding
Pro: the default behavior can be what we currently have: allow external and internal modification
Con: this could lead people to close their widgets off to outside access, making things more complicated than they need to be
Pro: not a big deal, you can always change the definition of the Widget.Type - and who are you to say that you know better than the author of the widget in the first place?
Con: it is one additional source of runtime errors
Pro: you could say that this is actually a way to reduce sources of errors since you can be certain that you don't accidentally change Properties from the outside that should never be changed
Pro: even if the user forgets that a property cannot be connected to / changed, it won't crash or anything and the user will be able to rectify the code easily
Con: but the runtime cost! the branching!
Pro: fuck off

> Original text from Trello:
>Looking back at the C++ Widget class, we can see that the Widget has a few additional fields to what the current "Widget" has:
> 
> 1. Opacity: float
> 2. Visibility: bool
> 3. Offset: xform
> 4. Claim: claim
> 5. Grant: size2
> 6. Xform: xform
> 7. ChildAabr: aabr
>
> Our current approach would be to put all of them into properties. The more properties I have, the more visibility the user has into what is going on. You can, for example, react to a Widget becoming invisible.
> However, properties can also be changed by the user. And for example, the grant should only be set by the parent (actually, by the layouting process).
> 
> Here's what I would do:
> 
> 1. Opacity: clearly a Property, clamped between 0 and 1. It can be set by user-defined code in the Widget and read by everybody.
> 2. Not a thing. We can determine that a Widget (and all of its children) will not be drawn and then prune that subtree from the Design but if you want to hide a Widget on screen, you set its opacity to 0. Done.
> 3. Property again. The offset transformation is happening outside the layouting process and is freely controllable by the user.
> 4. A different thing entirely. Don't know yet. But not a property.
> 5. Maybe an output?
> 6. Internal. But a "Scene aabr" (which is equal to what a "window aabr" would be if the scene fills the entire window) should be an output.
> 7. Notsure. Output?
> 
> One problem I see here is that 5, 6(scene aabr) and 7 could be properties, but only if you cannot set them. Then again, they could be outputs, but only if they can be read from the inside and auto-emit the last value to new receivers.
> Both of those things are not really a perfect match for this use-case. Of the two, a private Property would be the better choice... 
> Are private properties something that we want?
> 
> Alternatively, we can update outputs to always hold the last value they emitted and furthermore allow a Widget.View to inspect that value without connecting to it.
> But what now is the difference between this and a Property? I guess the cached value of an output is not serialized with the Widget, but that's really more of an implementation detail. And we still have the problem that the user code is not allow to emit from the output, no matter if it is cached or not.
> 
> We could make them Emitters on the Widget's Layout.
> Alternatively: Aliases of values on widgets, like soft links, dynamic except for the ones that alias internal values

Property states:

public: external modification / internal modification
external: no internal modification
internal: no external modification
built-in: no internal / on external modification

Widget Definitions should not be allowed to define built-in properties.

I though about whether State should be a private property as well, but it shouldn't because it is a true implementation detail.

## Properties as a single Value

> Original idea: Design should be immutable so we don't ever need to lock the event queue
> However, That's actually not true since we need to lock at some point in order to update the design. After that, the design isn't touched again until after it has been drawn.

what if all Widget Properties are in fact a single Value (a Map) and every Property just operates on a single field of that Value? If the Value is immutable, we could pass the "current Value + RenderCallback" to the renderer and don't need to lock anything.
That still leaves the question of how we represent the Scene hierarchy.
I mean, we could simply have that be a Value as well...
But, here's the thing: I already wrote extensively against using Values to replace pointers.

What about this: at the point of rendering, you do not traverse the tree of widgets and call their render function to create a design?
Instead, you build up a tree (Plate, Blueprint..?) of the triplet {Property Value,  Render Callback, Children List } and then run the render callbacks on that instead?
That sounds very good.
We _don't_ have a Value representation of the actual Scene, only of the parts that are rendered. There are no pointers in the Document and you never work "with" it, instead all you do is call a pure (recursive) function on it:

```python    
def draw(design&: Design, properties: Value, render: RenderCallback, childen: List[Triplet]):
    design.add(render(properties))
    for child in children:
        draw(design, child.properties, child.render, child.children)
```

I like it.

Perhaps the Triplet should be a quartet that also includes the maximum size of the resulting subdesign for better parallelizability? 
And/or the widget ID (pointer?) to allow partial updates of the Design.
I guess what we could also do is to simply execute the RenderCallback, see what the subdesign looks like and then decide whether we can update the existing subdesign or if we have to create a new section in the Design to hold it. This way we should get a behavior much like a std::vector in that increasing the complexity of a subdesign will cause it to re-allocate, but making it simpler will just re-use the existing space.
Of course, with that approach we also need to communicate to the Design when a widget has been removed.

Sidenote: it should be possible to draw multiple Widgets in parallel if we manage to make the drawing function constant (and forbid the creation of new Widgets, connections etc.)