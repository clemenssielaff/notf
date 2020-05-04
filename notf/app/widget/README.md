AnyWidget
=========

The `AnyWidget` class's purpose is to be a common base-class for all Widgets. It itself is an implementation of the
`Node` template, meaning that it itself has properties, slots and signals. This way, we can use pointers to any widget
and be certain that is has a minimum set of attributes.

Widget Spaces
-------------

In order of application:
    1. Offset
    2. Layout
    ...
    n. Window

In order to display the Widget on screen, it is first transformed by its own offset transformation, then by the
layout transformation of the parent. The parent itself is most likely another Widget, meaning it too will apply
its own offset transformation first, followed by the layout transformation of its own parent. And so on, until
the parent is the root widget and the iteration stops.

Every Widget stores 2 transformations: the Offset and the layout transformation. The Offset is a property, the layout
transformation is an internal value that can only be set by the parent layout.

The offset transformation is meant for things like a "shake animation" or maybe shrinking the widget while it is being
clicked on with the mouse. Nobody but the Widget itself know about the offset. It does not change the Widget's claim and
therefore does not affect the parent's layout in any way. The only thing affected by the offset transformation is the
way the widget is rendered.

The layout transformation is defined by the parent layout and is used to place the widget among its siblings. It
is a full transformation since the layout may decide to apply a perspective transformation or whatever to it children.
The widget itself does not actually do anything with the layout transformation, it is only more convenient to store the
value in the widget itself, than it is in the layout. If you want to get the window transformation of a widget, for
example, it is a lot easier to get the parent and get its layout transform (which you know where to find at compile
time), than it is to find the layout, check if and where it stores the transformation for each of the children and get
it from there.

Widget Layout
-------------

Layouting is the process of ordering and transforming children of a widget. The order determines which child widget is
drawn on top of other ones and the transform determines how (in relation to its parent) the child widget is placed,
rotated or otherwise transformed.

This operation is orthogonal to the type of widget. For example, you might want to have a widget that displays a list
of images, but may also switch to a [cover flow](https://en.wikipedia.org/wiki/Cover_Flow) layout on request. Obviously,
you don't want to destroy the entire subtree and create a new list widget, just to accomodate the change in layout.
This is why every widget contains a `unique_ptr` to a `Widget::Layout`, a virtual class that has a raw, mutable
reference to a widget and is used to lay out its children.

It is important that a Layout can not exist outside its Widget, so that it may never *outlive* its widget. In order to
ensure this, the Layout cannot actually be taken, it can only be changed from the outside. Of course you still need to
be able to change layout-specific values, but only through a temporary reference.

When changing the Layout of a Widget, the constructor of the new Layout is passed the `unique_ptr` of the old one. This
way, the new layout can actually be a transition between the old and the next Layout.
