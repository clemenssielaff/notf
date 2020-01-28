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
  
Every Widget has 0-n child Widget in no particular order, addressable by a Widget-unique name. New Widget can only be created by their parent Widget, and only be removed by their parent (except the root, which is created and destroyed by the Scene). When a parent Widget removes one if its children, the child will remove all of it children first before destroying itself.  
It is not possible to change a Widget's parent as that would change the Widget's Path. You can however change its position among its siblings since that operation does not influence how the Widget would be addressed in the Scene. For more details see Widget.Path and Z-value below.

One basic design decision is that parent Widgets have explicit knowledge of their children, but child Widgets should never rely on the presence of a particular parent. This way, we can build re-usable atomic Widgets that are easily composable into larger ones. Think of child Widgets as an implementation detail of the parent.

> **What is a Widget**  
> Originally, everything was a Node. I assumed that Widgets, Services, Resources, Layouts and Controller objects would all be subclasses of Node and share the same hierarchy. I also assumed that the user would be able to derive from the Node class to define custom Node types.  
> However, one by one, the other use-cases for most Node classes disappeared. Resources are better managed somewhere outside the hierarchy, where they can pre-fetched, cached and replaced without having to worry about their place in the Scene. Controller objects were replaced with the logic Circuit, Services are now globally accessible from the Application. And Layouts became a field inside a Widget, not a Node itself. At that point, all Nodes were Widgets.


## Widget Definition and Type

Up until quite recently in the design process, I assumed that the user would be able to subclass the Widget base class to define custom Widget types. This is a natural thing to do in C++ or any object oriented (OO) programming language, but for notf it posed some unique challenges. First of all, defining a subclass means that the user would be allowed to define custom members and modify them during the class' virtual methods -- all user Scripts were potentially stateful. Furthermore, in order to create child Widgets of a custom type you need to pass the subclass as an argument in an interpreted language which means that you required access to the dynamic type objects and the interpreted context in which they live.  
Both things did not sit well with the purely data-driven approach that I was going for.

Fortunately, I discovered that it was possible to contain all user-define code in `Script` objects, basically strings. At least for an interpreted runtime. Statically compiled Scripts would compile with the rest of the program, but I figured that the true power of notf is first and foremost in its live updates, which require (?) a runtime. If you indexed each Script by a name, you basically broke down the whole "subtype" into a map of string to string. Combined with other POD (Value Schemas and the State Machine, see below), this lead to the invention of the Widget.Type object. Widget.Types are objects that describe the Widget, much like a Schema describes a Value. The contents of the Value might change, but the Schema stays the same.  
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
  Functions that can be attached to an input plug
* Free *Property Callbacks* (Dict[str, Property.Callback])
  Functions that can be attached to a property
* The Widget's *StateMachine* (StateMachine)
  The StateMachine controls which Callbacks are active, and provides the initial set-up Callback for the Widget type.

For more details on Functions (and Callbacks and Scripts) see the chapter on "Widget and Scripts".


## Widget Name and Path

Unlike Circuit Elements, which have a Circuit-unique numeric identifier, Widgets are uniquely identified and addressable using their "Path". The Path of a Widget is simply the concatenation of all ancestor Widgets' names, separated by a reserved delimiter symbol. Prerequisite for this is of course, that all Widgets have a name that is unique within the parent Widget. Widget names are not allowed to contain Path control symbols. The consist of:

* `.` denotes the furthest Widget in the Path up to that point or "this" Widget, if it is the first name. `..` means "go up one level".
* `/` is the name separator. The name to the left is the parent of the name to the right if none of them are control symbols.
* `:` is the separator between the Widget Path and the target Element (see below).

By having the identifier of a Widget be another POD object (in this case a string), user Scripts are free to address Widgets all over the Scene. Paths in user Scripts can be absolute or relative. Relative Paths are resolved by replacing the initial dot with the absolute path of the Widget executing the Script. Much like filesystem paths in Linux, you can use the reserved `..` name to go up a level in the Scene.

Example of a valid Widget Path:

* `/absolute/path/to/a/widget`
* `./child1/../child2/grandchild38` (if called from `/parent` resolves to `/parent/child2/grandchild`)

Additionally, a Path can also be used to address a single Circuit Element (Input Plug / Output Plug or Property) on a specific Widget. Formatted as a string, an example for a Path to an Element would be: `/parent/widget:element`. The name of the Element is the last token in the Path and is separated from the rest by the element symbol `:`.

We do not differentiate between Paths leading to a Widget and those that lead to an Element because the functions that take a Path as an argument are specialized to return a specific type anyway:

1. `get_widget(Path)`
2. `get_property(Path)`
3. `get_input(Path)`
4. `get_output(Path)`

Inside each of the function, the Path needs to be tested whether it leads to a Widget or Element of the requested type anyway. Having an additional "Element Path" sounds good from a statically typed point-of-view, but since Paths are basically deserialized from a string input by the user, we do not gain anything from parsing two different types.  
Note that the `get_input` and `get_output` functions above return both the respective Input/Output Plugs as well as Properties, since Properties can act as inputs _or_ outputs (more an that below). Since all three share the same namespace, you can simply ask for a Input/Output Plug first, and then for a Property should no Plug exist. 


## Properties

Conceptually, Properties are a single Value associated with a Widget instance. They are declared as part of the Widget.Definition and have a name that is unique among the Widget.Type's Elements (Input Plugs / Output Plugs / Properties). A Property has a constant Value Schema and is initialized with a default value which must not be empty. They can act as both Receivers and Emitters in the Circuit but can also be read and written to directly by Scripts living on the Widget (but not from the outside).

We say Properties are single Values "conceptually", because they actually refer to entries in a single _Property Map_ Value that lives inside the Widget. Storing all Properties inside a single Value simplifies the rendering of a Widget (see chapter on rendering) and is generally easier to work with than a variable number of Values stored in a C++ `std::unordered_map`, for example.

Widgets store no data outside their Property Map. There might be additional state stored in Proprety Operators (see next chapter) but that is not accessible from the Widget level.

Properties can be updated both from Scripts executing on the Widget and from an Emitter upstream in the Circuit. In the second case the update Value will be wrapped in a ValueSignal. Still, we have decided against adding another Callback to the Property that would allow the Signal to be accepted or blocked -- instead, Properties that are updated from upstream Emitters will unconditionally take the Value but "ignore" the Signal (meaning, do not "accept" or "block" the Signal). Properties are all about data, there is no good use case in which they would try to control the flow of an incoming Signal like an Input Plug might.

There is another wrinkle to Properties in comparison to "pure" Circuit Emitter/Receivers. In the Circuit, the Receiver has full control over which Emitters it wants to connect to, while Emitters act as if they were unaware of the Receivers they emit to (apart from a function to test if there are any Receivers at all). Properties do not change that specific behavior, but they do allow any Widgets to initiate a connection to the Property of any other Widget. Internally, the connection is still initiated by the Receiver, but the Widget owning the connected Property will be unaware of what happened.  

Why would we want other Widgets to force their Values onto the Properties of other Widgets?  
Let's take a progress bar as an example. The parent Widget is called "ProgressBar", it has a Propert called "progress" and is the thing that is known to the rest of the UI to display the progress in whatever form it wants to.  
Inside, the "ProgressBar" Widget you find a rectangle Widget that grows to the right, the "bar" of the ProgressBar. It has has a "width" Property, which is a real number in the range [0, 1]. Next to it is a label Widget that displays the progress in text and has a "percentage" property in the range [0, 100[. Both children have no idea where their respective progress values comes from, they only know how to display it. The parent Widget's job is to orchestrate its children, so it must forward the progress value to its children, even though they have no idea that they are part of a bigger Widget.  
In order to push Values down to its child Widgets, the "ProgressBar" needs to connect to their Properties since the children do not know about the parent Widget and its "progress" Property.

Of course, by allowing essentially random Widgets to connect to and overwrite Propreties on any other Widget, we open the user up to all sorts of error cases, where Property Values change suddenly between Functions and you don't know where the modification came from. In order to hand control back to the user, we added the `external` argument to Property Operations.


### Property Operation

Apart from their Value Schema and name, a Property is defined by an optional Operation that is performed on every new value. Like in an Operator in the Circuit, the Operation can either choose to return the input Value unmodified, change it in some way (clamp it, round it to the nearest integer ...) before returning it or returning `None`, in which case the existing value of the Property remains unchanged.  
Unlike Operator.Operations, Property.Operations have access to a Widget.View object. This way, Operation are able to read their own and other Properties of the same Widget which allows for a Property which is a number that is clamped between two others, for example.

Property Operations cannot be changed as they are as much part of the Property "type" as its Value Schema.

Like Operator.Operations, Property.Operations are allowed to store an arbitrary Value as private data. This allows the Operators to keep semi-private values that are only relevant for their internal workings. For example, you could only allow the new value if it is larger than the average of the last three. Without the ability to store data own their own, this average would have to be a Property on the Widget, which is not really where it belongs.

As mentioned before, the third argument to a Property Operation (besides the Widget.Handle and the private data Value) is a flag `external` that is set to `true` if the new value was received from an upstream Emitter in the Circuit and to `false` if the new value was set from a Function on the Widget.  
This offers a more powerful way for the user to determine how a Property reacts to inputs from different sources than for example a simple `is_private` flag would do (where "private" Propreties could not be changes from outside their Widget). For example, if you wanted to allow all non-empty lists Values from the outside, but want to allow the Widget itself to set an empty list, you could simply reject all empty lists if the `external` flag is true. 


### Property Callback

Like Input Plugs, Properties also have an optional Callback that is invoked after the Property has been updated. Unlike the Property Operation, which is only allowed to operate on the value, the Callback has full access to a Widget.Handle, which allows it to update Properties and emit from Output Plugs.  
Which Property Callback is active at any given time is defined by the Widget's current State. The Property.Callback can also be set to `None` to disable.

Of course, you have to take care not to create any infinite loops between Callbacks, just like you do with any emission in the Circuit. If a Property Callback of a Property were to set the same Property again, it would create an infinite loop and an immediate deadlock if it was not for the reentrancy check in a Property's `Emitter._emit` methods that will catch this case and fail gracefully. The same goes for the "wider" cycle, in which two Properties keep changing each other's value forever.

There is however one exception to the "no cycle" rule, one that is only possible because of the separation between the Property Operation and -Callback: Imagine a Widget with two Properties `A` and `B` that need to be kept in sync. Let's say that `B=A+2` must hold at all times. Whenever you set `A` to `x`, its Callback will also set `B` to `x+2`. But there is no way to differentiate this case from one where you changed `B` instead of `A` and so `B` will continue to update `A` as well ...which would cause a NoDag exception. But while this is a cycle, it is clearly not an infinite one. In fact, after touching both `A` and `B` once (no matter which one is set by the user), no further progress will be made. We can catch this case with a simple all-purpose optimization: check whether the new value is equal to the existing one *prior* to checking for cycles. If the value is the same, just do nothing and return. In this case, setting `A`  to `x` will change `B` to `x+2` which will in turn cause `A` to update to `x` again. But since at this point, `A` already has the value `x`, it will simply return nothing and the cycle is broken.


## Input Plug

Apart from Properties, Widgets have another way to receive input from the outside world: Input Plugs. Unlike Properties, Input Plugs do not store any state beyond what Callback they are currently deferring to. An Input Plug is an implementation of a Circuit Receiver and reacts to ValueSignals that are passed down from connected Emitters upstream. All other Signals are ignored, except for the automatic disconnection from completed Emitters, which a Receiver does automatically.  
When an InputPlug receives a ValueSignal, it will forward the Signal to an InputCallback inside the Widget. Which InputCallback is active at any given time can be changed during State changes of the Widget's StateMachine. The InputCallback can also be `None`, in which case the Signal is simply ignored.

Input Plugs are the point at which input events are handled by a Widget. If the user clicks into the UI, the "mouse-down" Emitter will emit an event Value which is forwarded to receiving Widget Input Plugs in decreasing order of their Widget's Z-value.

InputCallbacks differ from other Functions in a Widget.Definition insofar as that they are passed the handle to the Widget as well as the mutable ValueSignal that was received. This way they can  "accept" or "block" a ValueSignal, should they decide to do so.


## Output Plug

Output Plugs are Emitters that live on a Widget and can be invoked by Scripts on their Widget to propagate non-Property Values into the Circuit as needed. Output Plugs are truly stateless (apart from the state held by the Emitter base class). They share a namespace with the rest of the Circuit Elements that are discoverable on a Widget (Input Plugs and Properties).

Since they do not interfere with the Widget that they live on, I originally planned that Output Plugs could be told to emit from anywhere. This way, they could act as aggregators, with multiple Widgets emitting from the same Output Plug on some third Widget.  
This approach did however go against the principle of encapsulation, stating that child Widgets should never rely on their parent Widgets for functionality and that child Widgets are basically an implementation detail of their parents. Basically, we cannot emit from an Output Plug of a child Widget since that is an implementation detail and we cannot emit from an Output Plug of a parent Widget, since that should be unknowable to the child.


## Widgets and Scripts

When designing notf, I assumed at first that the user would add custom functionality by deriving from types that are supplied by the architecture. However, as discussed in "Widget Definition and Type", this approach meant a lot of overhead to supply base classes to runtime bindings (for Python, Lua, JavaScript etc.) and the introduction of untracked state. That in turn required the introduction of special "serialize/deserialize" functions and limited our ability to introspect the state of the entire UI (which is not too important right now, but would be, if we decide to implement ["event sourcing"](https://martinfowler.com/eaaDev/EventSourcing.html) or multi-threaded event handling). Lastly, the more I read about functional programming and immutable values, the more I liked the idea of designing the UI using _pure functions_ only. A pure function only operates on a given set of inputs, must return a value, and does not produce any side effects.  
This is how I eventually settled on the concepts of _Functions_ (upper-case _F_).

A Function is a piece of user-defined code, with a fixed list of parameters and a single return type. The code must run in isolation and operate only on the given arguments -- in other words, be pure functional. Throughout this document I also use the term _Script_, and where a Function always comes with a fixed signature, a Script is just any piece of user-defined code. A Function contains a Script, but a Script is not a Function.

So far, there are 5 types of Function defined in the whole architecture. One in the Circuit and four in the Scene:

1. Operator.Operation `(input: Value, private_data: Value) -> Optional[Value])`
2. Property.Operation `(input: Value, private_data: Value, widget: Widget.View, external: bool) -> Optional[Value]`
3. Property.Callback `(new_value: Value, widget: Widget.Handle) -> None`
4. Input.Callback `(signal: ValueSignal, widget: Widget.Handle) -> None`
5. StateMachine.Callback `(widget: Widget.Handle) -> None`

Note that we have introduced two new shorthands here: An _Operation_ is a Function that returns a Value whereas a _Callback_ does not. To recap:

* **Script** -- A user-defined string from the user that contains code that can be executed by the application's runtime.
* **Function** -- A Script that is embedded in a function with a fixed signature and return type.
* **Operation** -- A Function that returns a Value.
* **Callback** -- A Function that returns None.

Unlike in previous iterations of the notf architecture, the user is never able to access other Widgets directly. The only way to interact with other Widgets is to connect/disconnect to/from one of their Input/Output Plugs or read a Property -- all of which can be done using the respective Path.


---

This is how far the document is consolidated, after this point things get messy.

---

## Widget State Machine

A State change goes like this:

1. StateA:Enter Callback is called. StateA is the entry State of the Widget.
2. Somehow a transition to StateB is triggered.
3. StateA:Exit Callback is called
4. StateB:Enter Callback is called


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

---

what do we do about exceptions thrown in a State enter/exit Callback?
this actually generalizes to the bigger picture of exception handling. The current approach is to ignore and report errors and continue to the best of our ability but that might not actually be the best. 

Instead, what if any error interrupts the execution? The program stops and presents the error including the stack and everything. The big feature is that the whole event can then be replayed after the scripts have been updated by the user. In order for that to work, we need atomic events, that can be completely rolled back if they failed. And every call to a service must also be stopped. 
This is actually easy to do. Instead of firing commands off to a service when the script calls for it, you queue the command to be sent after the event has finished. This way we can simply discard the queue so far in the case of an exception during event handling. 
The roll back is a bit more involved, but only because for that feature we need event sourcing and for that we need the single- state application like in redux. 

This is for a debug mode btw. In release mode I think that the current approach suffices, where exceptions are handled ( reported and ignored ) as locally as possible. 

Another advantages of event sourcing are:

* find where in the history of the application a property was set to x / an input was fired etc.
* automatic and guaranteed valid undo for the UI. That means only commands to the services need to provide user-defined undo functions. And that we certainly can't automate.

---

What do we have:
To recap; i want to be able to have atomic events, meaning that an event either succeeds (no exception is thrown) or it fails (an exception is thrown).
We have no half-states, where the event succeeded up to a point but then failed.
If the event succeeds, it update the UI and all services.
If the event failed, it will be as if it never happened.
This should be really great for debugging, since the program can stop when an exception occurs, tell you where it happened and then let you change exiting Scripts before re-running the event.
Similarly, you could even step back in time, change a few scripts and then let the events re-run.
The question here is, what events? I'd figure that clicks into the UI are not too useful, if you are changing the location of child Widgets, for example - in that case you'd want the Widget's reaction to an event to be replayed, even if it has moved away from the position of the original mouse click ...
Why am I even questioning this?
I think, the problem is that I am still not fully convinced that this feature is necessary.
It will cause a lot of rewrite and a little bit of runtime overhead when it exists, but I think I'm mostly worried about an increase in complexity of the backend.
Not of the user-facing stuff, that should become easier ... but I am not really convinced of that yet.
So we have two questions:
1. what features would a persistent, redux-style, atomic application object allow us to implement?
2. how would we even implement it?
The two are closely interwoven, but I think I will be able to answer the second question easier.
The first question has the possibility to drift off into "wouldn't it be nice" territory, where features sound good but make no sense (see "replay" above).
Okay, so how would be implement this.
In redux, we have a single object containing the entire application state.
That's nice and if it is a persistent object with state-sharing, we have an entire history of the application with minimal space requirements.
One problem with persistent objects is that we cannot really have pointers, can we? 
It seems we can. For example you could use an immer::box<std::unique_ptr<T>>. But then, you cannot modify T anymore.
I mean, that's the crux behind immutability.
Whenever you change a single value anywhere in the application, the change needs to bubble up all the way to the top object, if we store the application as it is right now: as a tree.
You update a leaf, and then you need to update the owner of the leaf, and its owner and so on all the way to the application object, which is then the next ground truth.
Okay, let's have a look at what state we are storing.

**At the very bottom, we have Values.**
Values are easy, because they are basically immutable values by design. Nothing magical here.

**Then we have the Logic.**
The Circuit is basically a map ID -> Element
The Element can either be an Emitter or a Receiver (or both or a subclass of any or both)
How can we represent that in data? Subtypes can have arbitrary fields...
OMG, do we need to use a Map str -> Any here? That would be terrible. 
But Any objects are definitely not const, so it would be a map str->str. 
An you know what? If we did that, we would basically operate on a _fucking single string_, like an HTML document.
Well, fuck.
And we haven't even gotten to the real interesting part, the Scene hierarchy.
Then again, Widgets are probably easier, since they are all the same type, but differ only in Values.
Hmm...
You know what? I don't actually need the `Any` type at all. Right now, there are actually only a very limited number of *concrete* classes.
Okay, let's back up and enumerate.
Easy. The only concrete and persistent class (that excludes Signals etc.) in the Logic module, apart from the Circuit, is the Operator.
Another wrinkle to consider here though, is that the Circuit down not _own_ its Elements, it only offers an easy way to access them.

**Now to the Scene.**
Here, we get to the question of ownership.
Right now, parents own their children, but if we represent this structure 1:1 in data, we'd get the deep tree that we are trying to avoid. 
Instead, we want to keep everything as shallow as possible.
Tables come to mind, like in a database.
We could give each Widget a unique ID, like we do with Circuit.Elements. You wouldn't need them to refer to another, that is still done via a Path, but in data, these IDs act as primary keys like in a DB.
Okay, I just did a bit of sketching and I am pretty sure that it works. Like in a DB you have multiple tables with primary keys as ID and foreign keys as pointers.
Whenever you update a callback or a Value or whatever, you just update a single table. The Application state is just the collection of all tables. 
One type per table.

Let me dedent this, because I think I know now how this could work.
Tables are the right approach. They are proven and easy and work great with immutable data.
One problem that I see is that of speed. Right now, we have structures in memory that point to each other. I'm thinking parent Widgets, child Widgets, and the up/downstream of Receivers and Emitters.
Would it be possible to use IDs not at unversally unique IDs but as the number of the row in the table? 
I think that would be possible. 
The obvious "problem" here is that the table would grow uncontrollably, if we would never re-use rows. 
So we have to do that.
Right now, we can already determine when a Widget or Element goes out of scope, let's just do the same here.
If a row is deleted, it is actually not modified but it's number is added to a "free list" 
Optionally, we could also set a flag on the row saying that "this row has been deleted - if you access it, you are basically segfaulting" ... maybe in debug mode only though.
When you create a new Widget or Element or whatever, you just pop the last entry off the free list and use that instead.
Actually, better than a "this has been deleted Flag", add a "Generation" number. You start out with generation 1, when that is deleted you set it to -1. When the row is re-used, set it to 2, then -2 on deletion again. This way you can check whether the row is currently alive by checking if `generation > 0`. Still only relevant in *our* debug mode (because if that fails, then the user of the software won't be able to fix it).
I AM FUCKING LOVING THIS

Okay smartiepants:
If all of that is so great, then tell me: how does ownership work with that DB model?
A whole lot more manual, that's for sure.
I think, you need to have another row for reference counting. 
The good news is that you don't need to use an atomic variable, because recount modifications should happen on a single thread.
You *do* need atomic refcounts for the rows and the tables (for if we want to do multi-threaded event handling), but immer should provide those out-of-the-box.

Hey Hey!
Another good thing about this: diffing tables should be a whole lot easier than diffing trees.

For reference, this is how I imagine a tabel to look like in c++ code:

```c++
struct Widget {
    immer::box<std::string> name; // internalized string?
    uint parent;
    immer::array<uint> children; // maybe immer::vector but probably not
    uint type; // foreign key into Widget.Type table
    uint current_state; // index of a state defined in the type's state machine
    // ...
};

struct WidgetRow {
    // row number is implicit
    int generation;
    uint refcount;
    Widget widget;
};

using WidgetTable = immer::vector<WidgetRow>;

```

Note that I store the current state in the Widget, not in the StateMachine.
This way we allow the StateMachine to be stateless.
The same would go for the Callbacks stored in the Widget. I would leave them in the Widget.Type and only reference them by index.
Of course that does exclude stateful things, like the active Callback of an Input or Property, or the data of a Property.Operation - those would be stored in the Widget itself.
... But would they?
Let's take the simple question of Properties and their values.
The type of Properties is determined by the Widget.Type. No need to store that information in the Widget.
But the Property value belongs to the Widget instance, so we should store it.
Well, the number of Properties won't change. At least not often (only if the Widget.Type changes and at that point we are dealing with a new Widget type anyway. That should only happen during development anyway).
We could store Properties in an array of Values. Which index belongs to which Property can be read from the Widget.Type.
This approach translates well to all other questions, like Property Operation data, active Callbacks for Inputs and Properties etc.
Even for the state machine, was it not for the fact that there can always be only a single one and therefore the array of active states always has a length of 1.

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

---

You should be able to change the render callback of a widget based on state.

Invisible widgets must be marked as such so they don't get mouse events.
Otherwise, we might have scrolling widgets that are outside of the cropped rect of their parent in the location of a mouse event for example.

Note that this is different from being "disabled" (which should be a state, rather than a built-in thing)

Widgets should differentiate between the "is_visible" property and "is_viewable" method. Visible widgets can still be not viewable with an opacity of zero.

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


# Layouts

more thoughts on layouts, yaaaaii!
Here's where I am currently at.
We've just discovered the neat and delightful data approach of putting every thing into fucking tables, maybe wrapping a few classes around it to pretend that we are still in happy OO land.
At this point, I honestly want to start programming in C++ (because for this particular thing, Python won't do) but we are not done yet. | Note: there are python libraries for persistent data structures too, so it might actually work
In fact, I think we should better wait with the C++ part until we have a fully working UI in Python, without all the table goodness.
True, we won't have the full feature set that I would image, but if we started the C++ rewrite now, I am pretty sure we would end up running into design problems fairly quickly.
And those were always way easier to solve in Python than in C++.
That means, before we start carving the design into stone (C++ stone, that is), we should first know how it will look like.
In full.
And that ... includes Layouts.
And a few other things, but so far I think that Layouts are the only thing left in the Scene module.
Maybe Design... but honestly, that already somewhat orthogonal to the rest, even though it will live (?) on the Widget.

Okay, with Layouts I am at a point where I think that a Layout is _not_ a Node, but an abstract object that is held by a Widget.
Here, we run into the first problem. Abstract objects are not really a thing in table-land.
Okay, let's take a step back.
A Layout must provide three things: xform of the widget in parent space, grant size in local space and child aabr in local space. 
However, how it provides these are up to the individual Layout.
Furthermore, Layout has its own state. For example: is a FlexLayout wrapping? It certainly does not make sense for all Layouts to have an `is_wrapping` field. 
Okay, that's one thing.
Two of the three things (xform & grant) are set by the parent layout for each Widget, the last one is set by the layouting process of the widget's own layout -- point is, they are all set at once. There are no separate functions for all of them. That's good.
So we can say that a Layout has a single Function and state. An Operation, so to speak...
The Operation's signature is:

    (child_claims: List[Claim], available_space: Size2, data: Value) -> Tuple[List[Tuple[Xform, Size2]], Aabr]  # (child [xform, grant], children_aabr)
    
Problems:
  P1. The list is ordered, yet I thought it was the Layout's job to provide Z-ordering. What's going on?
 ~P2~. Before I had explicit or implicit claims. Implicit Claims were defined by combining child Claims. This should probably also be done in the layouting Operation, because where else would it be done?
   P3. How do we handle clipping of child Widgets? Being invisible seems to be a good thing to know as early as possible, because we don't need to render or layout invisible Widgets. 
    
    
**P2**: That sounds like we need a fat object that is pre-loaded with a few defaults to pass into the Operation.
Let's call it ... a Composition
The Composition has the following fields that are initialized to default values:

    Composition:
        + children: constant List of...
        + claim: constant Claim
        + xform: mutable Xform (default=identity)
        + grant: mutable Size2 (default=preferred size from Claim)
        + child_aabr: mutable Optional[Aabr] (default=None)
        + claim: mutable Claim (default=widget claim)
    
If the Operation does not change the `child_aabr` but leaves it at its default=None, the final step is to calculate the child_aabr from the `children` field of the Composition.
We assume that in some cases, the Operation might have better insight in how a child Aabr is calculated (or maybe you want it to be a bit larger, smaller etc?), but if the user does not provide this, we can do it ourselves as well.

That would change the Operation's signature to:

    (composition: Composition, available_space: Size2, data: Value) -> None

Assuming that we can have mutable arguments (the Composition).
But yeah, we need that. Otherwise, how would we do the painting if not for a Painter object? I'm pretty sure that all relevant runtime languages will support that.

**P3**: There is an easy answer here: 
After the Operation has finished, check for each child Widget whether the Aabr you get after applying the transformation to the grant size2 overlaps with your own grant Aabr in local space (meaning the bottom left is at zero).
And if it doesn't, the Widget is clipped. Done.
That would make many things easier. Rendering and event propagation.
Actually, event propagation is separate from this since I don't dive into the hierarchy...
Of course, you'll have to be clipped by something, the Window at the latest.. But maybe not the parent? Not always at least?
What if there is an "is_clipping" flag on the Widget?
What if there is an optional clipping shape attached to each Widget? That defaults to the rect spanning from zero to the size of the Widget?
That would be awesome.
But also really expensive, wouldn't it?
Yeah, but also *really* good.
I mean, I always knew that I needed a "shape" in addition to the raw bounding rect, just to know if a mouse click even hit or not.
We could use that Shape to clip.
But you might want to use another one.
Where are we at?

    * Widgets need a shape that acts as a hit-zone for mouse events
    * Widgets can have a clip shape that can be equal to the hit-zone (but does not have to be) or can be empty in which case the widget won't clip its children at all
    * Widgets need a third shape that surrounds every shape drawn by the Painter to represent the widget - this shape is used to determine whether a Widget has become invisible. 

Interestingly, I don't see the bounding rect at all anymore.
In order to do hit detection, we need the final, clipped shape in Window coordinates. Question is, do we update it whenever the Layout changes or whenever the shape is asked for?
What about a compromise. In addition to the two shapes, a Widget *does* store a bounding rect in window coordinates, but one that is automatically updated and totally invisible to the user.
The bounding rect is calculated automatically from the hit-zone shape.
This bounding rect is always updated whenever the layout changes and is empty, if the Widget is fully clipped - which is convenient because we don't need an additional flag.
With the Z-order and the Aabr in window space, we should be able to limit the number of failed shape tests when finding the next Widget to propagate a mouse event to.
Let's step through the algorithm to find out whether a mouse click hit a particular Widget.
    
```python
def next_handler(click: ClickEvent, candidates: List[Widget]):
    """
    A Python generator function that takes a click event and produces the next interested Widget if there is one.
    
    :param click: The mouse click event, containing the position of the click in window coordinates.
    :param candidates: All Widgets that have subscribed to receive mouse click events, ordered from front to back.
    """    
    # Create a cache to store the calculated clipping shapes.
    cache: Dict[Widget, Path] = {}
    
    # We start with the topmost candidate and continue until somebody either blocks the event 
    # or we run out of candidates.
    for candidate in candidates:
        
        # If the click is not even in the Widget's bounding box in window coordinates, 
        # there is no way that this candidate will handle the event.
        if click.window_pos not in candidate.hitbox:
            continue
        
        def get_hitshape(widget: Widget) -> Path:
            # If we have already calculated the hitshape of the current Widget, just re-use that
            if widget in cache:
                return cache[widget]
            
            parent: Optional[Widget] = widget.get_parent()
            if parent is None:  # widget is root
                result: Path = widget.hitbox
            
            else:
                parent_hitshape: Path = get_hitshape(parent)
                widget_xform: Xform = widget.get_xform()  # transformation in parent space
                result: Path = parent_hitshape  # TODO: THIS IS WRONG, WE NEED TO CLIP THE CHILD HITSHAPE IF ANYTHING
            
            cache[widget] = result
            return result
        
        # If the click is inside the candidate's hitshape, this is the next Widget to receive the event
        if click.window_pos in get_hitshape(widget):
            yield candidate

    # No more candidates.
    return

```

We do need some constraints though, or at least have a good think about them.
For example, do we allow Widgets to expand their hit-zone into areas that aren't even drawn? The inverse I can see (hit-shapes that are smaller than the painted-shape), dropshadows for example are drawn but are not part of the hit zone.
Does the clipping shape only apply to children? Or does it apply to the widget itself as well?
I would say it should apply only to children. This way the Widget can draw itself around an area reserved for its children. 

Okay, last push for today - this is a lot more involved than I'd hoped.
We have 3 shapes: draw, clip and hit.
Ideally, I want an order where `a >= b >= c`, meaning `b` is fully enveloped in `a` etc.
Why do I want that? Because ... actually, let me just enumerate what I want.
We can put these 3 into a matrix, which is always fun:
    
         |     Draw      |      Clip     |      Hit      | (within the same widget)
    ======================================================
    Draw |       X       | D grows to C  |  independent  |
    ------------------------------------------------------
    Clip |  independent  |       X       |  independent  |  
    ------------------------------------------------------
    Hit  |  independent  |  independent  |       X       |
    
In the first row, we can see that the draw shape needs to encompass the clip but not the hit shape.
It needs to encompass the clip shape, because we need the draw shape to determine whether or not a Widget is visible.
If it draws its widgets outside of its own draw shape, the widget itself may not be visible, but we can also not discard it during rendering, because that would apply to all children as well.
We also explicitly dedided against clipping a widget against its own clip shape.
It however does not need to encompass the hit shape, because you can have a hit shape larger than what's shown on screen (like buttons on a touchscreen)
The clip shape only applies to children and is not influenced by any other shape (but influences the draw shape).
The hit shape is completely separate and does not influence the other shapes at all.

Things get a bit more interesting when we look at the shapes' relationship with the parent Widget's shapes
    
         |    X Draw     |    X Clip     |     X Hit     | (parent widget, transformed into child space)
    ======================================================
    Draw |  independent  |    D <= XC    |  independent  |
    ------------------------------------------------------
    Clip |  independent  |    C <= XC    |  independent  |  
    ------------------------------------------------------
    Hit  |  independent  |    H <= XC    |  independent  |
    
    Everything of the child widget is clipped against the clipping shape of the parent.
   
or slightly differently:

         | Draw  | Clip  |  Hit  | (within the same widget)
    ==============================
    Draw |       |   a   |   b   |
    ------------------------------
    Clip |   a   |       |   c   |  
    ------------------------------
    Hit  |   b   |   c   |       |
    
Since we are dealing with shapes within the same widget, all shapes have a two-way interaction:
a) We want the draw shape to be able to be larger than the clip shape because we might want to frame child widgets clipped by the parent.
We do not however want the clip shape to be larger than the draw shape because we might otherwise determine that a widget is not visible (since its draw shape is fully clipped) and ignore the Widget's subtree during rendering.
Even though its children, that are clipped to an area outside the draw shape, would still be visible.
In a way, the parent Widget is deferring "painting" its child Widgets, and whatever area is allocated for the children to paint has to be assumed to be fully filled.
b) The hit shape can be smaller than the draw shape, if you want to draw a button with a dropshadow for example you want the button clickable, but not the shadow part.
At the same time, we want the draw shape to be smaller than the hit shape, for example if you have a touch screen interface and you want the touchable area to extend a bit outside the button shape.
c) The hit shape of a widget can be larger than its clip shape, since the clip only clips the children, not the parent.
=> Can the clip shape of a widget be larger than its hit shape? I don't see the connection between the two but is there a use case?
       
         | Draw  | Clip  |  Hit  | (parent widget)
    ==============================
    Draw |   a   |   b   |   c   |
    ------------------------------
    Clip |   d   |   e   |   f   |  
    ------------------------------
    Hit  |   g   |   h   |   i   |
    
Since all shapes of the parent are unaffected by that of a child, all effects here are imposed onto the child shapes (if there are any).
a) Since we are not clipping to the draw shape (or bounding rect) of the parent anymore, the child draw shape can also be bigger than the parent's.
b) The child's draw shape is clipped to the clip shape of the parent.
c) No effect.
d) No effect.
e) The child's clip shape is clipped to the clip shape of the parent.
f) No effect.
g) No effect.
h) The child's hit shape is clipped to the clip shape of the parent.
i) No effect.

---

**Layouts are not Nodes**
Just access the layout fields from the Widget.View and that's that

---

* Every Widget owns a Layout Node.
* Scripts have (read-only) access to the Properties of the Layout via the "property getter" function of a Widget.View/Handle - setting a layout property will fail. This way we don't have to implement a new concept like read-only Properties, we can simply safeguard against setting them in the Widget specific handles for Scripts.
* (apart from the read-only access to its properties), the existence of a Layout is invisible to user Scripts.
* Layout property names are considered "reserved" for Widgets and cannot be used in a Widget.Type.
* Layouts are owned by a parent Node. That Node can either be a Widget or another Layout. The parent is irrelevant for the Layout and it should never need to access it.
* Layouts have an associated set of child Widgets that are laid out by it.
* Layouts provide a function: `get_child_widgets(where: Optional[Union[Vec2, Aabr]] = None) -> List[Widget]` that contain all child Widgets at the given point ordered with descending Z-value (later child Widgets are drawn behind earlier ones)
* If you call `get_child_widgets` without a point or aabr, all child widgets are returned in order with descending Z-values.
* Layouts provide a property `child_aabr` which contains the AABR of the union of all child widgets in widget space. The property is updated whenever the layout changes and is accessible to Callbacks of the associated Widget.
* Layouts produce a data structure containing, for each child widget: { An xform in parent space, The grant size}. We need this to be a data structure in order to allow for transitions between multiple Layouts.

---

We started out with a list, right now we have a Dict[str, Widget] to own the children. This is great for lookup but does not provide an order.

That's okay though, because that is what the layout is for.  The Layout should be a tree of Layouts - a tree of only a root if there is only one Layout; a tree of three, if there is a transition from one to another (the third one would be the parent, merging the two); of five, if you transition from the previous transition into a new state etc. Default is one.
I guess, you can also have a single Layout at all times, in the case of a transition you simply have a special `TransitionLayout` that contains two other ones. 
Layouts have state ... properties, you might say. ... OMG are Layouts maybe Nodes?

We could have Widgets both parent other Widgets as well as a (usually root-only) subtree of Layouts. Layouts would *not* be part of the Scene hierarchy, meaning they cannot parent other Nodes... I think that might work.

Also this would get rid of property mutability flags since the props would live on the layout not the widget and cannot be set from the outside, and we forbid the external connection into properties anyway. 

--- 

Lists may not be the one fits all solution to store child nodes. In a long scrolling list, a deque would be better and I've already talked about quad tree child locator strategies for a QGraphicsScene-like widget.

Everything should work as long as you can produce a strong order from it
And maybe an efficient lookup by name, if the path becomes an important part of the design

(from other note):
Add a "ChildLocator" strategy object to Widgets. A unique_ptr that is null by default. If it is null, the Widget will just iterate through all items and find the first (brute-force). If it is not null, it will be updated every time the layout is changed. For example, we could have a quad-tree that is updated after each layout change (independent of the layout). It can then also decide when to re-balance.

Interesting aspect: the child locator strategy is tightly tied to the layout of a Widget, which is already a strategy-like object

---

problem: layouts need a list of widgets to lay out.
the order of the list is important as it will determine the widget's placement in the layout.
i think, i will be able to condense ever layouting operation to that of a list: a flex layout stacks from left to right (or whatever), an overlayout or free layout from front to back, and a table in row- or column-major order.
Previously (and most generally, I assume) the order in which a widget lays out its children is constant.
In fact, I went so far as to store the child widgets of a widget in a list and only forward that list to the layout.
When you wanted to re-order child widgets in the layout, you had to re-order them in the child list of the widget.
This has the added advantage that you don't have to involve the Layout when you iterate over widgets to render them, as they are already sorted in the correct order.
Furthermore, this allows a Widget to calculate its Z-value relatively inexpensivly.
Well, that seems to me like there are good reasons to keep an ordered list of child widgets stored in the widget itself.
The order of child widgets can be changed by the user through a widget handle, not by any outside widget including the child widgets themselves.
That means, you can change the order in an Input Callback, State Enter/Exit Callback or Property Callback.
Every change in the order of widgets triggers a relayout.

There is another place from where you can change the order of children, and that is in the Layout itself. 
Changes made in the Layout do not trigger a re-layout themselves (obviously).
This way you can have (for example) a music cover view like in itunes, where the center child is always displayed on top of all others, even though it is Xth in a list.
Wait a second ... let's examine that example.
If the Layout changes the order of children in order to move a widget _in the middle_ of the list into the foreground ... the moved child widget will be at the front of the list at the next layout.
That sucks.
I guess, the Layout can "override" the Z-value of a Widget?
But then we don't have the nice proprety that the list of child widgets is a 1:1 representation of the draw order... :(
Oh well. I don't think there is a way around that.
That means that the Layout _cannot_ change the order of child widgets, it can however override their z-value.
-
I like the approach in Qt, where you can manually set a Z-value override on a Widget with a signed integer.
The default is zero for all Widgets.
Positive values are drawn in front of all smaller values.
Two widgets with the same value are drawn in the order they appear in the list.
-
Fortunately, the algorithm to create a back-to-front odering form a list of widgets with z-overrides should be fairly trivial.

```python
def get_draw_order(widgets: List[Widget]):

    # sort widgets into buckets by z override value
    buckets: Dict[int, List[Widget]] = {}
    for widget in widgets:
        z_override: int = widget.get_z_override()
        if z_override in buckets:
            buckets[z_override].append(widget)
        else:
            buckets[z_override] = [widget]

    # concatenate a new list from the old
    return list(chain(*[buckets[z] for z in sorted(buckets.keys())]))
```

And since this is somewhat annoying to do for every single widget every time, we can probably cache the result if we change the signature to take (widgets: List[Widget], z_overrides: List[int]) and doing some (Memoization)[https://en.wikipedia.org/wiki/Memoization].
This should be particularly easy to do, since we can store both arguments as persistent values and just check for equality.
... Then again, that would require us to keep a list of z_overrides on the parent widget, which is not really feasable since the z_override is something that we'd might set on the widget itself? 
Yes, I think so.
But maybe there is a way to speed this up. Maybe we can correlate a set of "moves" from the list of z_overrides? I mean, a list of [0, 0, 0, 0] clearly means "take the input widgets and return them unchanged", whereas a a list of [0, -1, 0, 0, 1, 0] means " take the second element, then the first, then the third, forth, sixth and finally the fifth.
Actually, the whole function can be just that, no access to the Widget class required:
[0, 0, 0, 0] -> [0, 1, 2, 3]
[0, -1, 0, 1, 0] -> [1, 0, 2, 4, 3]
That looks cachable enough, especially if you consider that we can make the z-override only 1 byte wide and it should still work. I mean, seriously. You don't need that many.
With that optimization, the first argument is basically its own 64 bit hash, whereas the second one needs one call to hash_combine with (0, -1, 0, 1) and (0, 0, 0, 0).
And you know what? Maybe we "should" store the z-override in the parent Widget, even though it is accessible from the child.
We only need to make sure that if you reorder the child list, you also reorder the z-override list.
Basic.

---

If Widgets store the position amongst their siblings, it should be easy to recalculate their z value. Similarly, changes to a widget's list of children will only affect the children and will not propagate downwards. 

However, that does not solve the problem that we need a list of children in draw order for rendering. Maybe we need the information twice?

1. In the parent widget: list of all children in draw order
2. In each child widget: its position among its siblings 

... a bimap - at least conceptually

---

We had implicit and explicit claims before - What if we have any implicit layout horizontally ( to fit a list of widgets into a column) but explicit vertically? Maybe the implicitly should be part of the claim?
