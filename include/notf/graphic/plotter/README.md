Painting Widgets is arguably the most important visual task of notf. In order to maximize performance, the Widget
drawing pipeline is divided into several stages, each of which helping to avoid unnecessary work.

# Painter
Is passed in as a mutable reference to `Widget::paint`, where it modifies the...

# WidgetDesign
A data object containing information on how to paint a Widget. Is used so that we don't have to call `Widget::paint`
for every Widget, every time a new frame is drawn but only for those that actually changed visually. The WidgetDesign
is basically bytecode that is eventually ingested by the...

# Plotter
The Plotter behaves very much like the `Painter`, but instead of being driven by the user (with an interface suited
for human interaction) it is driven by the Painterpreter. The Plotter ingests all information necessary to draw a
WidgetScene, performs some internal optimizations and eventually passes them on to the GPU for rendering.

The Plotter uses OpenGL shader tesselation for most of the primitive construction, it only passes the bare minimum
of information on to the GPU required to tesselate a cubic bezier spline. There are however a few things to consider
when transforming a Bezier curve into the Plotter GPU representation.

The shader takes a patch that is made up of two vertices v1 and v2.
Each vertex has 3 attributes:
    a1. its position in absolute screen coordinates
    a2. the modified(*) position of a bezier control point to the left, in screen coordinates relative to a1.
    a3. the modified(*) position of a bezier control point to the right, in screen coordinates relative to a1.

When drawing the spline from a patch of two vertices, only the middle 4 attributes are used:

    v1.a1           is the start point of the bezier spline.
    v1.a1 + v1.a3   is the first control point
    v2.a1 + v2.a2   is the second control point
    v2.a1           is the end point

(*)
For the correct calculation of caps and joints, we need the tangent directions at each vertex.
This information is easy to derive if a2 != a1 and a3 != a1. If however, one of the two control points has zero
distance to the vertex, the shader would require the next patch in order to get the tangent - which doesn't work.
Therefore we increase the magnitude of each control point by one.

Caps
----

Without caps, lines would only be antialiased on either sides of the line, not their end points.
In order to tell the shader to render a start- or end-cap, we pass two vertices with special requirements:

For the start cap:

    v1.a1 == v2.a1 && v2.a2 == (0,0)

For the end cap:

    v1.a1 == v2.a1 && v1.a3 == (0,0)

If the tangent at the cap is required, you can simply invert the tangent obtained from the other ctrl point.

Joints
------

In order to render multiple segments without a visual break, the Plotter adds intermediary joints.
A joint segment also consists of two vertices but imposes additional requirements:

    v1.a1 == v2.a1

This is easily accomplished by re-using indices of the existing vertices and doesn't increase the size of the
vertex array.

Text
====

It is possible to render a glyph using a single vertex with the given 6 vertex attributes available, because there
is a 1:1 correspondence from screen to texture pixels:

0           | Screen position of the glyph's lower left corner
1           |
2       | Texture coordinate of the glyph's lower left vertex
3       |
4   | Height and with of the glyph in pixels, used both for defining the position of the glyph's upper right corner
5   | as well as its texture coordinate

In order for Glyphs to render, the Shader requires the FontAtlas size, which is passed in to the same uniform as is
used as the "center" vertex for shapes.
