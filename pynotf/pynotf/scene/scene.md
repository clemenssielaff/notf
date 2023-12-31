# The Scene

The Scene takes its name from the "Scene Graph" that is inherit in many game engines or DCC tools. It is older than the Circuit in its conception and is a higher-level abstraction to the workings of a UI. The Scene is conceptually somewhat orthogonal to the Logic, but the Scene module depends on the Logic module and not the other way around.

Data-structure wise, the Scene is a tree of Widgets. There is one RootWidget in a Scene, and like every other Widget, it can have 0-n children. Every Widget has a single parent that outlives it and cannot be changed. When the Scene is removed, it will safely destroy all Widgets from the leafs up, meaning that the root is the last Widget to be destroyed.

A Scene owns a Circuit and manages it in a way that allows the user to inject user-defined code (Scripts) at very specific places that are as expressive as possible while still being safe. All modification of a Scene (and its Circuit) is explicitly single threaded and should occur on a dedicated thread. You can have parallel loops or similar technicalities where they make sense, but they must operate on local data only. Much like Circuit Elements, the removal of a Widget in the Scene is delayed until the end of an Event.


# The Widget

A Widget is the basic building block of of the Scene's tree structure. Every Widget has a single parent, which is constant and guaranteed to be valid throughout the Widget's lifetime -- except for the root Widget, whose parent is always empty. The root Widget is created with the Scene and is the last Widget to be destroyed.
  
Every Widget has 0-n child Widget, addressable by a Widget-unique name. New Widget can only be created by their parent Widget, and only be removed by their parent (except the root, which is created and destroyed by the Scene). When a parent Widget removes one if its children, the child will remove all of it children first before destroying itself.  
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
  * All of the Widget's *Properties* (Dict[str, Property.Definition]).
  Properties need to be defined themselves, albeit just with a Schema and an optional Property.Operation. See below for details.  
  All Properties and input/output Plugs share the same namespace. Their names cannot be empty and must not contain Path control symbols.
* Free *Input Callbacks* (Dict[str, InputPlug.Callback])
  Functions that can be attached to an input plug
* Free *Property Callbacks* (Dict[str, Property.Callback])
  Functions that can be attached to a property
* The Widget's *StateMachine* (StateMachine)
  The StateMachine controls which Callbacks are active, and provides the initial set-up Callback for the Widget type.

> TODO: add layout(s?) to the Widget.Definition, once I have them all worked out.

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


## Widget Depth

Widgets can and do overlap a lot on the screen. Children always overlap with their parent Widgets but (depending on the parent's Layout) can also overlap with siblings. The order in which Widgets are drawn on screen and the order in which they receive "top-down" events like mouse clicks is determined by their "depth", a single floating point number that identifies a Widget's position among all other Widgets along the z-axis. Unlike the sibling order, which is only relevant to the parent, the depth value is global, meaning two Widgets can compare their values to determine their relative placement, no matter where in the hierarchy they are.

For a while, we considered using the depth value from the Widget's transformation in application space for depth calculation. While that would allow widgets to intersect on the depth axis, it would require turning our UI engine in to a full blown 3D engine. We eventually dediced to stick the the depth representation by order alone as it is easier for event handling, it is easier to understand when drawing and is less complex in code.

The root Widget has a depth value of zero by definition, all other depth values can be calculated by the following (recursive) Python snippet:

```python
def get_depth(widget):
    # Child Widgets are drawn on top of (with a higher depth value than) their parents.
    parent: Optional[Widget] = widget.get_parent()
    if parent is None:
        return 0  # widget is the root
    parent_depth: float = parent.get_depth()
    
    # The available space for child Widgets is the difference between the parent's and the nearest uncle's depth.
    grandparent: Optional[Widget] = parent.get_parent()
    if grandparent is None:
        available_space_per_child: float = 1
    else:
        available_space_per_child: float = 1 / (grandparent.get_child_count() + 1)
    
    # The depth of a child widget is determined by its position among its siblings and the number of siblings.
    # The lowest index is a 1, otherwise we would end up with a depth of zero for all widgets on the left edge of the tree.
    # We use sibling count plus 1, otherwise Widgets on the right edge of the tree could have a z-value greater than 1.
    local_depth: float = (parent.get_child_index(widget) + 1) / (parent.get_child_count() + 1)
    return parent_depth + (local_depth * available_space_per_child) 
```

The depth value is calculated anew every time you ask for it.  
While that might sound expensive, the alternative is worse: If we were to cache the depth value of all Widgets, they would need to be re-calculated every time a parent Widget changes the order of its children, or introduces a new child. Removing an existing child does not matter since the absolute depth values are irrelevant as that does not affect their order. And the calculation itself is not very complicated. It's just a lot of jumping around in memory. Maybe we could even get rid off a lot of jumps if we traversed the tree in a breadth-first fashion, which would allow us to get rid of most of the `getters` in the code above. Furthermore, you are able to cut the traversal short whenever you hit upon two siblings whose depth values did not change...  
The problem here is not efficient tree traversal but that the tree can grow large quite quickly. Whenever you add or move a new new first-level Widget, you would end up recalculating the depth values of all Widgets in the entire Scene. This is a problem because we need to minimize the maximum amount of work done during any event, not the absolute amount of work done across all events. The bane of UI is not throughput but latency.  

Note that the depth is not needed for Scene traversal, it is only required if you take two random Widgets in the Scene and want to know which one is drawn on top of the other.


### Child Widgets and Depth Override

We have already stated that any Widget can parent any number (including zero) of child Widgets. Each child Widget is owned by its parent and the parent is guaranteed to be alive for at least as long as the child is. Note that this description does not imply how child Widgets are managed by their parent (whether in a list, a map or something else).  
We have two use-cases for accessing a child Widget from a parent:

1. By name. For example, when traversing a Path. This is most efficiently done using a map [name -> Widget].
2. By render order. When rendering child Widgets, we need to iterate over all children from back to front (or front to back, if we can improve on the painter's algorithm). 

Clearly for 2. the map from 1. will not suffice, so we need a second way to list widgets in that specific order. In fact, in order to easily calculate the depth value for a Widget, it needs to know its position among its children. So we need the same information twice:

* A list in the parent Widget that contains all (visible) child Widgets in an order most convenient for rendering. Let's call it the "Render List"
* For each (visible) child Widget, its index in the RenderList. 
  
I am fine with the convention saying that all invisible Widgets have a depth value of 0. We do not need another way for the root to distinguish itself.

For a while I didn't like this approach because it feels redundant to store the render order twice, but then I realized that the actual duplicate space will (most likely) be fairly small. And any change will only affect the Render List and the direct child Widgets -- it will not propagate further.


## Widget Properties

Properties are a single Value associated with a Widget instance. They are declared as part of the Widget.Definition and have a name that is unique among the Widget.Type's Elements (Input Plugs / Output Plugs / Properties). A Property has a constant Value Schema and is initialized with a default value which must not be empty. They can act as both Receivers and Emitters in the Circuit but can also be read and written to directly by Scripts living on the Widget (but not from the outside).

Widgets store no Values outside their Properties. There might be additional Values that are part of Proprety Operators (see next chapter) but those are not accessible from the Widget.

Properties can be updated both from Scripts executing on the Widget, from an Emitter upstream in the Circuit and from Scripts executing on another Widget instance. In the second case the update Value will be wrapped in a ValueSignal. Still, we have decided against adding another Callback to the Property that would allow the Signal to be accepted or blocked -- instead, Properties that are updated from upstream Emitters will unconditionally take the Value but "ignore" the Signal (meaning, do not "accept" or "block" the Signal). Properties are all about data, there is no good use case in which they would try to control the flow of an incoming Signal like an Input Plug might.


### Forward connection

There is another wrinkle to Properties in comparison to "pure" Circuit Emitter/Receivers. In the Circuit, the Receiver has full control over which Emitters it wants to connect to, while Emitters act as if they were unaware of the Receivers they emit to (apart from a function to test if there are any Receivers at all). This could be called "backward" connection, because the direction of the connection is backward to the flow of data.  
Properties do not change that specific behavior, but they do allow any Widgets to initiate a "forward" connection to the Property of any other Widget. Internally, the connection is still initiated by the Receiver, but the Widget owning the connected Property will be unaware of what happened.  

Why would we want other Widgets to force their Values onto the Properties of other Widgets?  
Let's take a progress bar as an example. The parent Widget is called "ProgressBar", it has a Propert called "progress" and is the thing that is known to the rest of the UI to display the progress in whatever form it wants to.  
Inside, the "ProgressBar" Widget you find a rectangle Widget that grows to the right, the "bar" of the ProgressBar. It has has a "width" Property, which is a real number in the range [0, 1]. Next to it is a label Widget that displays the progress in text and has a "percentage" property in the range [0, 100[. Both children have no idea where their respective progress values comes from, they only know how to display it. The parent Widget's job is to orchestrate its children, so it must forward the progress value to its children, even though they have no idea that they are part of a bigger Widget.  
In order to push Values down to its child Widgets, the "ProgressBar" needs to connect to their Properties since the children do not know about the parent Widget and its "progress" Property.

Of course, by allowing essentially random Widgets to connect to and overwrite Propreties on any other Widget, we open the user up to all sorts of error cases, where Property Values change suddenly between Functions and you don't know where the modification came from. In order to hand control back to the user, we added the `external` argument to Property Operations (see next chapter).


### Property Definition

> TODO: Property Definition. I'm sure they exist...


### Property Operation

Apart from their Value Schema and name, a Property is defined by an optional Operation that is performed on every new value. Like in an Operator in the Circuit, the Operation can either choose to return the input Value unmodified, change it in some way (clamp it, round it to the nearest integer etc.) before returning it or returning `None`, in which case the existing value of the Property remains unchanged.  
Unlike Operator.Operations, Property.Operations receive a Widget.View object as an argument. This way, Operation are able to read their own and other Properties of the same Widget which allows for a Property which is a number that is clamped between two others, for example.  
Property Operations cannot be changed as they are as much part of the Property "type" as its Value Schema.

Like Operator.Operations, Property.Operations are allowed to store an arbitrary Value as private data. This allows the Operators to keep semi-private values that are only relevant for their internal workings. For example, you could only allow the new value if it is larger than the average of the last three. Without the ability to store data own their own, this average would have to be a Property on the Widget, which is not really where it belongs.

The third argument to a Property Operation (besides the Widget.Handle and the private data Value) is a flag `external` that is set to `true` if the new value was received from an upstream Emitter in the Circuit and to `false` if the new value was set from a Function on the Widget.  
This offers a more powerful way for the user to determine how a Property reacts to inputs from different sources than for example a simple `is_private` flag would do (where "private" Propreties could not be changes from outside their Widget). For example, if you wanted to allow all non-empty lists Values from the outside, but want to allow the Widget itself to set an empty list, you could simply reject all empty lists if the `external` flag is true. 


### Property Callback

Like Input Plugs, Properties also have an optional Callback that is invoked after the Property has been updated. Unlike the Property Operation, which is only allowed to operate on the value, the Callback has full access to a Widget.Handle, which allows it to update Properties and emit from Output Plugs.  
Which Property Callback is active at any given time is defined by the Widget's current State. The Property.Callback can also be set to `None`.

Of course, you have to take care not to create any infinite loops between Callbacks, just like you do with any emission in the Circuit. If a Property Callback of a Property were to set the same Property again, it would create an infinite loop and an immediate deadlock if it was not for the reentrancy check in a Property's `Emitter._emit` methods that will catch this case and fail gracefully. The same goes for the "wider" cycle, in which two Properties keep changing each other's value forever.

There is however one exception to the "no cycle" rule, one that is only possible because of the separation between the Property Operation and -Callback:  
Imagine a Widget with two Properties `A` and `B` that need to be kept in sync. Let's say that `B=A+2` must hold at all times. Whenever you set `A` to `x`, its Callback will also set `B` to `x+2`. But there is no way to differentiate this case from one where you changed `B` instead of `A` and so `B` will continue to update `A` as well ...which would cause a NoDag exception.  
But while this is a cycle, it is clearly not an infinite one. In fact, after touching both `A` and `B` once (no matter which one is set by the user), no further progress will be made. We can catch this case with a simple all-purpose optimization: check whether the new value is equal to the existing one *prior* to checking for cycles. If the value is the same, just do nothing and return. In this case, setting `A`  to `x` will change `B` to `x+2` which will in turn cause `A` to update to `x` again. But since at this point, `A` already has the value `x`, it will simply return nothing and the cycle is broken.


### Build-in Properties


Full Properties (can receive values, be written and read locally):
* Opacity (native): Number[0...1]
* Depth Override: Number[signed int]
* Xform (local): Xform3  
  Are only used by the Render Callback and do not influence the Widget's Claim.
* Claim: Claim

Read-only:
* Xform (layout): Xform3
* Grant: Size2
* Opacity (effective): Number[0...1]
* Index: Number[unsigned int]  
  Position among siblings in draw-order.
* Child Count: Number[unsigned int]

> TODO: do I need child count as a property? I think it should better be an emitter or nothing, actually.


## Input Plug

Apart from Properties, Widgets have another way to receive input from the outside world: Input Plugs. Unlike Properties, Input Plugs do not store any state beyond what Callback they are currently deferring to. An Input Plug is an implementation of a Circuit Receiver and reacts to ValueSignals that are passed down from connected Emitters upstream. All other Signals are ignored, except for the automatic disconnection from completed Emitters, which a Receiver does automatically.  
When an InputPlug receives a ValueSignal, it will forward the Signal to an InputCallback inside the Widget. Which InputCallback is active at any given time can be changed during State changes of the Widget's StateMachine. The InputCallback can also be `None`, in which case the Signal is simply ignored.

Input Plugs are the point at which input events are handled by a Widget. If the user clicks into the UI, the "mouse-down" Emitter will emit an event Value which is forwarded to receiving Widget Input Plugs in decreasing order of their Widget's Z-value.

InputCallbacks differ from other Functions in a Widget.Definition insofar as that receive the ValueSignal as a mutable argument as well. This way they can  "accept" or "block" a ValueSignal, should they decide to do so.


## Output Plug

Output Plugs are Emitters that live on a Widget and can be invoked by Scripts on their Widget to propagate non-Property Values into the Circuit as needed. Output Plugs are truly stateless (apart from the state held by the abstract Emitter base class). They share a namespace with the rest of the Circuit Elements that are discoverable on a Widget (Input Plugs and Properties).

> Since they do not interfere with the Widget that they live on, I originally planned that Output Plugs could be told to emit from anywhere. This way, they could act as aggregators, with multiple Widgets emitting from the same Output Plug on some third Widget.  
> However, this does not work with built-in output Plugs that are used to communicate true details about the Widget, like the number of children or the draw index as those need to be set internally from the Layout. If the user were able to emit from them, they could no longer be trusted.


## Widgets and Scripts

When designing notf, I assumed at first that the user would add custom functionality by deriving from types that are supplied by the architecture. However, as discussed in "Widget Definition and Type", this approach meant a lot of overhead to supply base classes to runtime bindings (for Python, Lua, JavaScript etc.) and the introduction of untracked state. That in turn required the introduction of special "serialize/deserialize" functions and limited our ability to introspect the state of the entire UI. Lastly, the more I read about functional programming and immutable values, the more I liked the idea of designing the UI using _pure functions_ only. A pure function only operates on a given set of inputs, must return a value, and does not produce any side effects.  
This is how I eventually settled on the concepts of _Functions_ (upper-case _F_).

A Function is a piece of user-defined code, with a fixed list of parameters and a single return type. The code must run in isolation and operate only on the given arguments -- in other words, be pure functional. Throughout this document I also use the term _Script_, and where a Function always comes with a fixed signature, a Script is just any piece of user-defined code. A Function contains a Script, but a Script is not a Function.

So far, there are 5 types of Function defined in the whole architecture. One in the Circuit and four in the Scene:

1. Operator.Operation `(input: Value, private_data: Value) -> Optional[Value])`
2. Property.Operation `(input: Value, private_data: Value, widget: Widget.View, external: bool) -> Optional[Value]`
3. Property.Callback `(new_value: Value, widget: Widget.Handle) -> None`
4. Input.Callback `(signal: ValueSignal, widget: Widget.Handle) -> None`
5. StateMachine.Callback `(widget: Widget.Handle) -> None`

> TODO: add rendering and layouting Callbacks here

Note that we have introduced two new shorthands here: An _Operation_ is a Function that has private a state Value whereas a _Callback_ does not. To recap:

* **Script** -- A user-defined string from the user that contains code that can be executed by the application's runtime.
* **Function** -- A Script that is embedded in a function with a fixed signature and return type.
* **Operation** -- A Function that has private state.
* **Callback** -- A pure virtual Function.

Unlike in previous iterations of the notf architecture, the user is never able to access other Widgets directly. The only way to interact with other Widgets is to connect/disconnect to/from one of their Input/Output Plugs or read a Property -- all of which can be done using the respective Path.


# The Atomic Application

At some point during the design we wanted to define how exceptions in Scripts would be handled by the running system. We know that all Scripts would be called somewhere below an Emitter.emit call in the stack, so it made sense to set up a general catch-all clause there and handle all exceptions as closely to the failure as possible.  
This is still a valid option, but one that comes with a major downside: it can leave the whole application (frontend and backend) in an inconsistent state if the user is not careful to handle exceptions gracefully (and if an exception escapes from a Script, we know that it was not). Imagine you have an application to rename a photo collection, based on the files' metadata. Your (pseudo-)code looks like this:

```python
# get a list of photos from a directory
photos = get_all_photos_in_directory("~/photos/")

# create a backup of all photos before renaming them
backup = create_backup_copies(photos)

# rename each photo
for index in range(len(photos)):
    photo = photos[index]
    date: get_photo_meta_tag(photo, "date")
    location: get_photo_meta_tag(photo, "location")
    photo.rename(f'{date} @ {location}.jpg')
    
    # and then remove the backup file
    remove_file(backup[index])
```

The exception occurs when one of the photos does not have a "location" tag, for example. Since it is not handled in the Script itself, the exception will interrupt the Script right in the loop and return immediately. At this point, we have an unknown number of already renamed files and a unknown number of backup copies, maybe in the same folder even? So that they get picked up as new "originals" during the next run of the script?  
Obviously, this is not great code. And you would be excused to think that it is the user's responsibility to make sure that code like this is never put into production. But the software is named "notf" for a reason, not "fewer tfs, though only if your code is perfect". Let's see if we can do better.

Ideally, what would happen here is that the exception interrupts the application and lets the user know where the error happened. The user would then have the opportunity to fix the code and let the event replay (if the user is the developer) or abort and make a ticket for the programmers to fix their shit. We focus on the first case, since we strive to be a great UI development tool first and a great UI framework second (as that naturally derives from the first).  
In either case, we want the event that caused the exception to not have any side-effects. Neither on the backend, nor in the frontend. 

The backend requirement is relatively easy to ensure: just queue all service calls and execute them in batch once the event has completed successfully. And if it fails, then just discard the queue. This behavior should be indistinguishable from the approach of firing service calls off immediately, since whatever effect they have will not be part of the current event anyway.

The frontend is a bit more involved. What this asks for are basically "atomic" events: events that either succeed completely (no exception escapes a ran Script) or fails without changing the application state at all. Whatever state changes have been made must be rolled back, as if they never happened.  
As I see it, there are two ways to go about this:

1. Keep a record of whatever changes are applied to the application state and undo them on failure.
2. Keep a snapshot of the original state around and re-apply it on failure, overwriting the current state.

The first option would allow us to keep the data representation separate from its modification. If you change the order of a Widget's children, you could simply store the modification itself (move the second child to the end of the list) and not worry about whether a child is a raw pointer, a Path or an index in a vector somewhere.  
On the other hand, it would introduce a lot more complexity into the code, since every new functionality that is supplied to the user would require a new "do" and "undo" function to go along with it, including the state that is needed to undo the operation.

The second option is the one favored by value-centric programming and modern frontend architectures like Redux. The first of [the three principles of Redux](https://redux.js.org/introduction/three-principles) is the idea that the entire application state is a single object. This, combined with an immutable object model allows for easy implementation of complete application serialization / undo-redo / copy / diffing etc. (as seen in the quite brilliant talk [Postmodern immutable data structures](https://www.youtube.com/watch?v=sPhpelUfu8Q)).  
This option would provide a superset of the functionality as the first, but instead of adding new Command-pattern like structures in the code, we would need to modify the way the data is stored in memory. So far, we have been refering extensively to the use of `Value` objects, which are (among other things) immutable. With Widgets however, we have introduced a whole lot of other state into the code that cannot easily be represented with Values ... or can it?

In order to have an application state that can be represented as plain old data in memory we would need to say goodbye to a few features that the previous programming model allowed us to do:

1. No more pointers. Since we no longer allow mutable fields, we no longer have memory identity for any object.
2. No more automatic lifetime management. With shared pointers, we did not have to worry about child Widgets getting destroyed when their parent goes out of scope. Without them, we need to do that ourself.
3. No more virtual base classes, all types need to be known since we cannot use `dynamic_cast`.
4. The current memory model is a tree, with parents owning their children and Widgets scattered all over memory in a large, branching application state. Combine that with a lot of small, local changes to individual Values, each change prompting another creation of all copy of the tree down to the change, and you get many many copies of minimally different data. Every time you update a leaf, you then need to update the owner of the leaf, and its owner and so on all the way to the application object, which is then the next ground truth.

For a while, I was not sure which way to go. The first one is easy to start, but is potentially hard to maintain because every new modification of any part of the application will require a new Command. We are basically creating and managing a lot of individual deltas, but without being able to outright copy the original state since it might contain pointers. So, on top of the maintenance hassle we have a lot of serialization at runtime.  
The second one seemed to offer some really cool functionality for free, but was totally incompatible with the way that the application was represented in memory...

Until it hit us: What if we could lay out the whole application in tables instead of a tree? Like in an SQL database. This way, we could keep everything in a data structure that was as shallow as possible. With structural sharing, we could even have an entire history of the application state with minimal space requirements! And unlike a tree, diffing two tables is basically trivial.
So, what do we get for our troubles?

* Better debugging: we can easily search where in the history of the application, a Value was modified, an Input Callback was fired etc.
* Automatic and guaranteed valid undo for the UI. Only Services need to provide user-defined undo functions - and those we certainly cannot automate.
* You could even step back in time, change a Script and re-run all Events up to now from that point onwards.
* Trivial serialization/deserialization: Instead of walking a tree, just dump a few tables to disk.
* Easy synchronization of the complete application state over the network (for example), if you have a multi-user session.

Very cool. Now to how this will actually work:

At the top, we have a single application object that stores a number of tables. Every row in a table represents instances of a single type: Widgets in one, Operators in another etc. That might sound unwieldly, but since we don't customize through subclasses (like in a regular OO program), there are only so many classes that we actually need to represent. In all of the logic module, for example, we only have two concrete (persistent) classes: The Circuit and the Operator. And you could argue that the Circuit does not even be its own table, since you only have one in a given application. And yes, there are more concrete classes than that in the logic module, but not _persistent_ ones. Instead we have Signals, Events, Errors etc.  
Whenever you update a Value, you modify its table and the application object -- and that is that. 

Instead of pointers, you have indices to a specific row of a table. Note that this does indeed mean that you need to know the exact type of object that you point to, so you can look it up in the correct table (unless of course we add another indirection but I hope that it will not come to that). Whenever a new object of the table's type is constructed, it will be put into the first free row of the table - initially at its end. Of course always appending new rows would mean that the table would grow forever, or at least until the application is shut down. This might not be a problem if you plan to keep the entire history of a session in memory, but if we decide to only keep the last 100 iterations around and serialize the rest to disk, this will no longer work. Instead, we need to re-use rows that have gone out of scope.

Fortunately, we know when a object goes out of scope because we have clearly defined ownership. Widgets are owned by their parent, Operators by their downstream. And for classes where ownership is truly shared, we need a dedicated row for a reference count (non-atomic, because modifications should happen on a single thread only). Whenever a row is deleted, it is not actually modified but its index is added to a "free list". When you create a new Widget or Element or whatever, you just pop the last entry off the free list and use that instead.
To safeguard against use-after free errors, every row stores a 'generation" number, that is different for every new object put into the same row. You start out with generation 1, when that is deleted you set it to -1. When the row is re-used, set it to 2, then -2 on deletion again. This way you can check whether the row is currently alive by checking if `generation > 0`. Note that this is only relevant in *our* debug mode, because if that fails, then the user of the software won't be able to fix it.

The number of a row acts as address of the pointed-to element. This row number is not exposed to the user, the user still needs to address Widgets with a Path and Circuit Elements with an ID combining the row number and generation value.

For reference, this is how I imagine the table for Widgets to look like in c++ code (using [immer](https://github.com/arximboldi/immer) for immutable value semantics):

```c++
struct Widget {
    immer::box<std::string> name; // internalized string?
    uint parent;
    immer::array<uint> children; // maybe immer::vector but probably not
    uint type; // foreign key into Widget.Type table
    uint current_state; // index of a state defined in the type's state machine
    immer::array<Value> property_values;  // values and callbacks of each property, the index in each array
    immer::array<str> property_callbacks; //  corresponds to the index of the property in the Widget.Type
    // ...
};

struct WidgetRow {
    // row number is implicit
    int generation;
    Widget widget;
};

using WidgetTable = immer::vector<WidgetRow>;
```

One approach for the table design (the one from the example) is to move as much state as possible into the `Widget` struct, which keeps all mutable state nice and centralized.  
Another approach would be to keep the tables lean, so that as little "old" data as possible is copied whenever a row is modified.

> TODO: return back to the design of the tables once we have the scene module all worked out


# Layouts

Every Widget with at least one child must have a Layout.
The Layout's job is to provide 5 data points for a (sub)set of the Widget's children:

1. Each childs' 3D **transformation** (position, rotation, scaling) in its parent space.
2. Each childs' 2D **grant** size.
3. Each childs' optional **depth override**
4. Each childs' optional **opacity factor**
5. The optional **axis-aligned bounding rectangle** (AABR) of all children of the Widget.
6. An optional **implicit Claim** that is the sum of all child Claims. More on that later.

In order to provide these data points, a Layout has a `Layout.Callback`, which is a Script that can be defined by the user. However, with Layouts it is quite likely that we will be able to provide all implementations necessary for most use cases; those would then be implemented in native code.  
The result of a Layout.Callback is written into a `Layout.Composition`, which is a structure:

    Layout.Composition: Tuple[
        widgets: List[Map[name: String, xform: Xform3, grant: Size2, depth: Optional[Number], opacity: Optional[Number]]],
        aabr: Optional[AABR],
        claim: Optional[Claim],
    ]

with

    Size2: Tuple[width: Number, height: Number]

    Xform3: Tuple[  # 4x4 matrix
        Number, Number, Number, Number,
        Number, Number, Number, Number,
        Number, Number, Number, Number,
        Number, Number, Number, Number]
    
    AABR: Tuple[Number, Number, Number, Number]  # left, bottom, right, top

    Claim: ... # see Claim

Each child's depth override and opacity factor (data points 3 and 4) are optional as they are provided by the child Widget's built-in properties. However, the Layout might choose to override the child's Widget depth-value or to multiply the Widget's opacity. If you don't touch these values in the Layout, they remain unchanged.

That the AABR and the Claim (data points 5 and 6) are also both optional as we could infer them from the per-child data points, by simply combining all of the child sizes and Claims. However, we expect that the Layout.Callback will have more insight into (and/or intermediate data for) calculating these two fields for their specific use-case. Or maybe you just want the child AABR to be a bit larger, smaller etc?  
In any case, if the user leaves these fields empty, we can calculate them ourselves as well.

I thought about splitting the Layout.Callback into multiple functions, but we cannot know which part of the Composition will change from the input alone. Therefore we would have to call each function on every new input. Furthermore, I expect the data points to correlate a lot, meaning that multiple smaller functions will have to do a lot of duplicate work. Therefore it is both cleaner and faster to have a single Layout.Callback to calculate the complete Composition in one go.

Layouts always contain a reference to their parent Widget, since they need to request the child Widgets' Claims for the Layout.Callback.


## Transitions

Layouts are separate from Widgets so that the user can change the Layout depending on the state of the Widget or the available size granted the parent Widget. Perhaps you want to display only the icons of push buttons if you shrink their parent Widget to its minimal width? Or arrange a horizontal list of checkboxes vertically if their available space becomes to narrow?  

Things become interesting when you consider that Layout switches should not (always) be immediate, instead you will most likely want a smooth transition between them. This is where _Transitions_ come in.  
A Transition is a built-in Layout that owns two other child Layouts that each produce their own Composition. Since Compositions are made up of known data, it is trivial to interpolate between them, no matter what internal process the child Layouts followed to produce their result.  
Of course, each one of the child Layouts can themselves be another Transition, and so on and so forth; although the utility of deeply nested Transitions is questionable.

Transitions are built-in, because they need to own two other Layouts, something that we do not allow for user-defined Layouts. We do however allow complete customization of the Transition process, this should therefore not be a problem in practice. In fact, I think the fact that a Transition is-a Layout can be completely irrelevant to the user.

Apart from the two children, Transitions also contain a reference to their (optional) parent Layout so that they can replace themselves with their target Layout on completion.  
Alternatively (if possible) they might actually "become" the target Layout. In that case, we can omit the parent Layout field.

In order to start a transition, the Widget has a method 
```python
set_layout(
    target_layout: Layout, 
    transition_duration: int = 0, # in milliseconds,
    interpolation_function: Optional[Transition.Interpolation] = None,
    warp_function: Optional[Transition.Warp] = None,
    )
```
that automatically replaces the current Layout with a Transition containing the current Layout as the *source* Layout and the given `target_layout` instance as the *target* Layout.  
Note that this works even if the current Layout is already a Transition.  
In the special case that `transition_duration` is 0, we can skip the transition and jump to the target immediately.

The Transition registers a timer Callback to be called once every frame for `transition_duration` milliseconds (it is called at least every frame; if the Widget's size is changed in between, the Transition layout might be called more than once). It then invokes the layout function on both of its children until the Transition has completed.

By default, a transition takes blends linearly between the *source* and *target*.
The rules for each child Widget are as follows:

* Xform: The 3D transformation is split into its translation, rotation and scale components. Their are lerped individually and finally merged together again, first the transformation, then the rotation and finally scaling.
* Size: Width and height are blended independently.
* Depth override: Since the depth values are integers, we'll have to live with the fact that they are going to jump at some point. I guess the middle of the transition is as good as any other.

Similarly, if the Layouts do not contain the same number of children, the source children missing in the target Layout will disappear once the Transition has passes its mid-point, whereas the additional children from the target Layout will only appear after the superfluous source children have disappeared.

Since a linear blend between the source and target state might not always be what you want, you can optionally define a `interpolation_function` that takes a single floating point value in the range [0 ... 1] and produces a Composition. 

If you only want to implement an easing blend, you can instead provide a `warp_function` that also takes a floating point value in the range [0 ... 1] but simply produces a corresponding floating point value. To allow a bouncing warp, we allow the returned value to be outside the [0 ... 1] range. In that case, the delta will be extrapolated beyong its source or target.

> TODO: maybe provide an `interpolation_function` only. If we add a free function `lerp(Composition, Composition)` to the context of an interpolation function, the user can simply calculate a warped blend value and then call `lerp`. This way we don't have two optional callbacks.


## Layout Properties

Unlike other Callbacks on a Widget, Layouts can not be pure functions. They cannot even work only with private data like some Operators do.  
Let's take the example of a simple ListLayout that takes a list of Widgets and arranges them vertically, one after the other. The spacing between the Widget is constant.
Where does the spacing value come from? It cannot live on the Widget, or at least it should not, since the Widget may also want to lay out its children in a StackLayout (for example), should the grant become too small to display a list. In that case, why would you have a "spacing" property on the Widget, even though it is not used anywhere? If we did that, the Widget.Type would need to contain a union of all Properties for any Layout types that the Widget may use. Not a great design.

Instead, Layouts need their own Properties. We use the name Layout.Property to differentiate them from Widget.Properties, which are conceptually similar but differ in implementation.  
Note also that Layouts are explicitly *not* Widgets themselves. Their ability to have other Layouts as children and the fact that they have (their own version of) Properties might look like they share much of the same concepts as Widgets but they really don't.

Layout.Properties can connect to upstream Emitters in the Circuit like Widget.Properies, but unlike Widget.Properties, Layout.Properties can also be set from the outside, from their owning Widget to be precise. Every change in a Layout.Property will trigger a relayout.

> TODO: this will be inefficient if you want to change multiple Layout.Properties at once. There is no way for us to _not_ update immediately and still ensure that the laid out values are correct when the user expects them to be. The only way I see here is to allow the user to temporarely block the relayouting, maybe using a RAII object (if that is supported).

Layout.Properties can have a Value.Operator to ensure that all of the Property values are valid. If a new value is not valid, the Property will not be updated and no relayout is taking place.


## Which Widgets to Lay Out

In the trivial case, a Layout would always take all children of its Widget. However, we might not have space enough for all children at all times, or maybe we just want to hide some Widgets without removing them from the parent.  
Therefore, in order to appear at all, every child of a Widget needs to be added to one (or more) Layouts explicitly. The "more" case occurs during Transitions, where the same child might have been added to both source and target Layout. Note though, that the child Widget will still only get a single xform, size and depth provided by the Transition, not multiple ones.

You might have noticed that the `Widget.set_layout` method takes a Layout *instance*, not a Layout.Definition. That is because we need to add Widgets to the Layout using `Layout.add_widget(child_name: str, data: Optional[Value])`. The Value.Schema of the second argument is defined by the Layout.Definition's `per_widget_data` field and contains widget-specific data that is available to the Layout.Callback.


The order in which the Widgets are added to the Layout is significant, because it will define the order in which they are laid out. For a ListLayout, for example, that might mean that earlier additions are further left than later ones. 

You cannot modify a Layout once it is set, but you can create a new Layout with new properties / children / per-child data and transition to that.


## Layout Definition

Like Widgets, Layouts must also be defined. A Layout.Definition contains the following fields:

```python
LayoutDefinition: Tuple[
    callback: Layout.Callback,
    properties: Dict[
        name: str,
        schema: Value.Schema,
        operator: Optional[Value.Operator]
    ],
    per_widget_data: Value.Schema,
]
```


## Layout Prologue

The child Widgets to lay out is defined by the user with a simple list of strings. When the Layout collects all Claims to pass to the Layout.Callback, it has to request each child by name from its Widget.  
To speed things up, Layouts should store handles to the child Widgets. With the data-in-table implementation of the application state, it should be trivial to determine whether or not a handle is still valid or not. The Layout will still need to request the Claim of each (valid) child Widget, but we do not have to do the lookup by name ever time.  
Invalid handles are quietly ignored.


## Layout Callback

The signature of a Layout.Callback is `(self: Layout.View, available_size: Size2) -> Composition`. The available size is what we call the "grant", since it is the space "granted" by the parent Widget's Layout.

Through the Layout.View, the Layout.Callback has access to all Layout.Properties as well as a map of per-widget data Values, indexable using the Widget's name as key. It has no further access to the parent nor to any child Widgets, like whether or not a specific Widget is selected or whatever. Information like that has to be forwarded to a Layout.Property.  
The reason for this limitation is that any knowledge of the laid-out Widgets would tie the Layout to the type of Widgets that it can handle, something that we explicitly want to discourage.

Every child Widget comes with its own depth-value override (see Child Widgets and Depth Override), ...
> TODO: how does the Layout handle depth overrides? Does it override them itself? Does it actually define a strict ordering of all child Widgets or is that a post-processing step that then actually resolves all overrides? Are there maybe two depth overrides, one from the widget and one from the layout with the layout one given precedence?


## Layout Epilogue

After a new Layout.Composition has been created, its values needs to be propagated to the child Widgets. In some cases, a relayout is triggered by the parent Widget itself. Here, it would be enough for the Widget to take the Composition and apply it to each child.  
However, changing a Layout.Property also triggers a relayout, and here we have no Widget in the call stack to accept the result and apply it to the children.  
Instead, the root Layout of a Widget must *push* the new Layout values to the Widget, which in turn then distributes it to the children. All of that can happen in native code and be completely invisible to the user.




## The implicit Claim

> TODO: Are implicit Claims even a good idea?
> modifying the claim of the parent is a dangerous game, because that change will propagate up, then the parent layout is
> called, which will propagate down again to the child which might again modify its claim etc.
> I guess that is okay only if having an implicit Claim.Stretch means that a change in grant will not cause a re-layout... 

Claims come in two "flavors": explicit and implicit. 


> **Alternative Layouting:**  
> And now a short story about an alternative Layout approach. I call it "alternative Layouting" because, like alternative facts or alternative medicine, it does not work.
>
> At one point during the design process, I became convinced that it should be possible to create a layout based on rules alone.  
> The idea here was that you would have a user Script that would return a set of constraints that define the Layout and would not have to be updated on every change of the parent's size.
> This also fit in well with the data-driven approach that I had adopted for the rest of the UI state.
> 
> And why not? With the introduction of Widget.Claims, we were already half-way there. You can think of a Claim as a collection of rules, now the only thing we needed was a more general expression of Layout rules. And I didn't have to look for long, because *anchors* already exist as a concept in layouting.  
> Simply speaking, an anchor is a constraint that places one item along the edge of another one. If you place the left and the top edge of an item to the bottom and right edge of another one, you have just "anchored" the first item to the bottom-right corner of the second.  
> Additionally, I was planning to allow the creation of "free" anchors, points or edges that Widget can be anchored to, that are encoded in the rules. For example, the iTunes cover view would consist of a list of free anchors (one per Widget), all placed on a horizontal line, with tighter spacing towards the edges and a central anchor in the middle. 
>
> In order to make the layout process as general as possible, we split it up into 3 phases:  
> In the first phase, a user-defined Script would be invoked to produce at least two rules for each Widget that was going to be laid out: one rule for the horizontal placement, one rule for the vertical.  
> In the second phase, we were going to build two DAGs, one for each axis in 2D space, defining the layout of each Widget in terms of constraints to attached or free anchors.  
> In the third phase, we would have distributed the available space in each dimension among the Widgets according to the constraints from step 2, so each Widget takes up as much space as possible but no more.
> Brilliant.
>
> There were however problems with this approach (if there weren't we would be using it).  
> 
> First, in the case of the iTunes cover layout, the first phase is already the complete layout process. There is nothing else but this one user-defined Script that we need to call. And we need to call it again, whenever the number of child Widgets change, whenever the grant of the parent Widget changes, whenever the cursor position changed... We can never skip this phase, becaues the user might have included a weird edge-case branch that is only executed if the layout has exactly 3 widgets, each 3.45 units apart ...   
Basically, we have added a lot of complexity, creating anchors and resolving them, but did not gain anything.
>
> Secondly, there was no general way to handle "overflow" in the third phase. If the Widgets were too wide or high to fit into the grant size, they would simply overflow. Everything else (like a wrapping Flexbox) requires special handling and the only way to do that without restricting flexibility was to add another user-defined Callback to split child Widgets into "groups" that could be laid out according to some orthogonal layouting process. And to make things worse, after this user-defined Callback had run, we had to repeat the whole process from phase 2 onwards, including (potentially) calling the same user-defined Callback *again*, after realizing that the new groups still didn't fit.
>
> All in all, this approach went nowhere. But I spent way too much though on it to not at least make a sidenote of it here.


# Widget State Machine

We have described a way to define a Widget through data and Callbacks. However, this only describes a single point in time for a Widget. Usually Widgets can be in different states: enabled / disabled / active / etc. These States only describe changes in business logic, not the internal Widget logic. A Widget can either be visible or invisible (depending on whether their internal _Visibility AABR_ is clipped by their parent _Clip Shape_, see "Shapes of a Widget" for details), but that differentiation is notf-internal and does not constitute an upper-case "State".  
States are defined by the user and toghether form a [Finite State Machine](https://en.wikipedia.org/wiki/Finite-state_machine).


> TODO: would it be a good idea to limit certain changes to a Widget instance (changes in children, in upstream connections, in the render callback, ...) to state changes? 

State changes, like value changes, are immediate. That means that the same Callback will execute in the context of both the source and the target state if it switches in the middle of its body.  
Switches states looks like this:

1. The EntryState:Enter Callback is called when the Widget is initialized.
2. ...
3. A State-changing Callback is started and runs up to the point where the switch is initiated.
4. The EntryState:Exit Callback is called
5. The NewState:Enter Callback is called
6. The State-changing Callback from 3. is continued.

This has the advantage that you can query and react to changes in state both before and after a State change but has the disadvantage of being more difficult to reason about (but not more so than any nested method call). That also means that you could trigger more than one State change from the same Callback.  
The alternative would be to restrict State changes to the Event epilogue. ... That might also work but I fail to see the advantage in the general case. Right now it seems like more complex code on our end and very little clarity added to the user's side. In fact, it might even prohibit certain functionality (having post a pre- and post-State switch part of the Callback) that might still prove to be useful.

Since we have no way of introspecting user-code, we cannot know in advance which State switches will occur. This would make it impossible for us to map out a transition diagram. Therefore, the user needs to define all reachable States from a given State up-front.


# Rendering

## The Render Callback

Whereas other Functions refer to an executable Script directly, the Render Callback of a Widget is not actually _on_ the Widget itself. Instead it is a free Callback that can be found by the name of the Widget.Type. The actual Script is looked up at runtime in the `RenderManager`, which contains a sequence of _Styles_. A Style is a simple map of Widget Type names to Render Calblacks. If the name is not found in the first Style, the StyleManager will attempt to find it from the second and so on until either a Style provides a Render Callback by the given name or the manager ran out of Styles, at which point this is an error. This is basically the "cascading" behavior in "Cascading Style Sheets" (CSS).

The idea here is that we want to decouple the inner working of a Widget from how it is displayed on screen. A button behaves like a button, no matter if its corners are rounded, if it has a dropshadow or even if it has text or not. And if you want to switch between one Style and the next, you should not need to have to restart your Application.

We should be able to connect all Widget Types with a Render Callback at startup and whenever the Style/s of an Application is changed. This way we can be certain that there will be no suprise failures at runtime. And you can actually serialize the current Render Callback for each Widget.Type with the rest of the Application state, meaning that if you do not change the Style at all (which I think is the default), you only occurr the cost of Render Callback lookup the first time you start a session.

Another level of abstraction is the Palette, which is separate from the Style. Within a Render Callback, you have access to the current Palette which can contain colors, fonts, measurements (like the average height of a button, font size), gradients or whatever. Palettes also cascade like Styles, meaning you can have a "default" Palette and override only certain characteristics with you custom Palette that has a higher priority.  
Palettes are separate from Styles. Wheras Styles contain Scripts (executable code), Palettes contain only data. Palettes are also easy to expose to the user of an application, because you can modify them using number entry fields / color pickers etc. and do not need to know the interpreted scripting language.

By associating the Render Callback with a Widget.Type, we ensure that the Render Callback of a Widget cannot be changed at runtime. You could easily imagine a world in which we _did_ allow that and it would also work, however we found that you do not _need_ to do it because every behavior that you get with multiple Render Callbacks can be duplicated by just having a big list of if/else statements at the root of you single Render Callback. The advantage here is that we contain the logic of drawing to the actual Render Callback itself and not spread it out over the union of all Widget Callbacks with a mutable Widget.Handle.  


## Accessibility

Another advantage of the separation between the Render Callback and the Widget is that it should be fairly easy to substitute on way of rendering with another. notf should be usable by all people that can use a computer. That means we also have to take care that the architecture allows easy communication of the state of a Widget as both a visual as well as a textual representation. For example a narrator. I have no idea how to do that yet, but we already have decoupled the visual representation of a Widget from its "logic", so I am pretty sure that we are on a good track here.


## Widget Design

The Widget Design is a list of Commands for the `Painterpreter` object that uses it to generate the draw calls to the GPU to render the UI. It is created by calling functions on a `Painter` object that is passed to the Widget's Render Callback.  
Example

```python
painter.set_fill_color("#ff0000")
painter.set_line_color("#000000")
painter.set_line_width(1)
painter.draw_circle(10)  # circle at origin with radius=10

painter.move(20, 0)
painter.draw_circle(10)  # a second circle 20 units to the right
```

Each Widget has its own Design, which allos us to update them independently from another. For rendering that means that we can use multiple threads without having to worry that the Designs might overlap in memory.


## Shapes of a Widget

The Render Callback of a Widget serves multiple purposes at once:

1. Create a Design object, a sequence of commands for the Painterpreter.
2. Create the hit Shape (defaults to the union of all painted shapes)
3. Create the clip Shape (defaults to None)

The **Draw Shape** is the outer edge around everything that is drawn as part of the Widget's Design, including its children (see Clip Shape). 

The **Visibility AABR** is used to determine when a Widget has become invisible. It cannot be manipulated by the user and is defined to be the narrowest axis-aligned bounding rect around the Draw Shape. 
It is the most performance critical of the Shapes and is used for broad-phase visibility testing. As soon the Visibility AABR is fully clipped by an ancestor, the Widget is considered invisible and will not be traversed during rendering or event propagation (unless the _Clip Shape_ is set to _unbound_, see below).

The **Hit Shape** is a shape in Widget space in which mouse events are received. If you have a circular button for example, you don't want the outer corners of the Widget's bounding rectangle to react to mouse events, even though the Widget's draw shape does not extend to there.
The Hit Shape is independent of the Draw Shape, because use-cases for Clip Shapes both smaller and larger than the Draw Shape exists: Dropshadows for example are drawn, but are not part of the Hit Shape; whereas the Hit Shape of circular buttons on touchscreen displays are usually larger than their Draw Shape. The Hit Shape is also independent of the Clip Shape.  
Defaults to the Draw Shape if not set explicitly.

The **Clip Shape** is Shape restricts all child shapes (draw / clip / hit) to a certain shape in parent space. The Clip shape does not affect its own Widget in any way. This way the Widget can draw itself around an area reserved for its children to frame them, for example. It has three states: default, explicit and unbound. 

* By default, the Clip Shape is the Visibility AABR. This should provide a good default behavior for most Widgets ans since it is an AABR I suspect that clipping can be very efficient.
* If set explicitly, the Clip Shape is independent of the Draw Shape. In that case, the Draw Shape will grow to encompass the Clip Shape.
* If the Clip Shape is left empty, it is said to be "unbound" and as a result the parent Widget does not clip its children at all.

We need the Clip Shape to be contained in the Draw Shape because the Draw Shape is used to determine whether a Widget is visible or not and we might otherwise determine that a widget is not visible (since its draw shape is fully clipped) and ignore the Widget's subtree during rendering. In a way, the parent Widget is deferring the painting of one of its regions to its child Widgets, and whatever area is allocated for the children to paint has to be assumed to be fully filled.

Do not set the Clip shape to "unbound" if you don't need to clip child Widgets because they are fully contained in the parent's Draw Shape already. The end result looks exactly the same as before, but if the parent Widget was fully clipped, we would still not know whether its children were visible or not.

If you have a Hit Shape larger than the Visiblity Shape (which is possible), the Hit Shape disappears as soon as the Visibility AABR is fully clipped. This might sound unintuitive but is usually what you'd expect from a visual interface element: if it is no longer visible, there is no way to interact with it.






























# NOPE
---




# Widgets and Logic

We have now established that Logic elements (Emitter and Receivers) must be protected from removal while an update is being handled. The update process is also protected from the unexpected addition of elements, but that does not matter for the following question, which is about the relationship between logic elements and Widgets.

First off, Widgets own their logic elements. This is elementary and one of the features of notf: a Widget is fully defined by the combined state of its Properties, while Signals and Slots are tied to the type of Widget like methods are to an C++ object. But do some logic elements also own their Widget? Properties should not, they are basically dumb values that exist in the context of a Widget but are fully unaware of it. Signals do not, they are outward facing only and do not carry any additional state beyond that of what any Emitter has. Slots however can (and often will) require a strong reference to the Widget that they live on. What now if we face problem 1 from the chapter "Logic Modifications" and Widget B removes Widget C and vice-versa? 

It is clear that an immediate removal is out of the question (see above). We still need to remove them, but only when it is absolutely safe to do so. And the only time during an update process when it is safe to do so is at the very end.
At the moment, the Widget is flagged for deletion, a strong reference to it is added to a set of "to delete" Widgets in the Graph. Then, at the very end of the update, we discard every Widget that has an ancestor in the "to delete" set and delete the rest. This way, we keep the re-layouting to a minimum. 

Note that it is not possible to check whether a Widget is going to be removed at the end of the update process or not, even though such a method would be easy enough to implement. The problem is the same as with an immediate removal. 
Depending on the execution order, the flag of Widget A would either say "this Widget is going to be deleted" if B was updated first or "this Widget is not scheduled for removal" if A is updated first. The result is not wrong, but essentially useless.
The same goes for anything else that might be used as "removal light", like removing the Widget as child of the parent so it does not get found anymore when the hierarchy is traversed.

---

# Widget Rendering Frontend

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
*Chrome Accessibility Fundamentals*
https://www.youtube.com/watch?v=z8xUCzToff8

https://www.w3.org/TR/wai-aria-practices-1.1/#intro

Here's another angle on rendering: What about invisible Widgets? Obscured, scissored, zero opacity or manually hidden ... there are many ways that a Widget is part of the tree but should not be part of the rendered Document. 
Of course, I have already thought of that, Widgets in previous versions had special built-in fields for that (in addition to their properties).
But if you have multiple views on a Widget, how can you ever say that a Widget is hidden? And zero opacity doesn't even make sense for a screen reader.

Then again, what does make sense in a universal kind-of-way? In the end, you have Widgets for the stated use-case of having a visual representation of data. If you don't have a visual representation (like for a screen-reader, for example) then why would you have a Widget in the first place?
I guess I am still not having a good idea of how a non-visual interface would work. For now though, I will simply ignore it, as brash as that might sound. The reasoning here is that we are building a _visual_ representation of data and then to say "how can we make it work non-visually as well" is the wrong approach. At some point it might be easier to have a completely new UI for non-visual users, should that become a problem.

> I have since come to the conclusion that UI is just one transformation applied to the Scene - the main one, probably, but only one. Widgets must take care to contain enough data to be represented in multiple ways (accessibility)




---

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




---

I like the approach in Qt, where you can manually set a Z-value override on a Widget with a signed integer.
The default is zero for all Widgets.
Positive values are drawn in front of all smaller values.
Two widgets with the same value are drawn in the order they appear in the list.

-----

I guess what we could also do is to simply execute the RenderCallback, see what the subdesign looks like and then decide whether we can update the existing subdesign or if we have to create a new section in the Design to hold it. This way we should get a behavior much like a std::vector in that increasing the complexity of a subdesign will cause it to re-allocate, but making it simpler will just re-use the existing space.
Of course, with that approach we also need to communicate to the Design when a widget has been removed.

Sidenote: it should be possible to draw multiple Widgets in parallel if we manage to make the drawing function constant (and forbid the creation of new Widgets, connections etc.)

-----

## Layout Properties

Layout property names are considered "reserved" for Widgets and cannot be used in a Widget.Type.

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
