from __future__ import annotations

from abc import ABC, abstractmethod
from typing import Dict, List, NamedTuple, Union, Iterable, Optional, Tuple
from time import perf_counter

import glfw

import pynotf.extra.pynanovg as nanovg
import pynotf.extra.opengl as gl
from pynotf.extra.utils import KAPPA
from pycnotf import V2f, Size2i, Aabrf

from pynotf.data import Value, Shape, ShapeBuilder
from pynotf.core.data_types import Expression


# PAINT ################################################################################################################

class SolidColor(NamedTuple):
    color: nanovg.Color


class LinearGradient(NamedTuple):
    start_point: V2f
    end_point: V2f
    start_color: nanovg.Color
    end_color: nanovg.Color


class BoxGradient(NamedTuple):
    box: Aabrf
    inner_color: nanovg.Color
    outer_color: nanovg.Color
    radius: float = 0
    feather: float = 0


class RadialGradient(NamedTuple):
    center: V2f
    inner_radius: float
    outer_radius: float
    inner_color: nanovg.Color
    outer_color: nanovg.Color


Paint = Union[SolidColor, LinearGradient, BoxGradient, RadialGradient]


# SKETCH ###############################################################################################################

class Sketch(NamedTuple):
    """
    A Sketch is the result of a Design interpreted by a Painterpreter.
    It is everything that the Painter needs to draw pixels to the screen, but is high-level enough to persist between
    frames.
    Ideally, you would update Sketches only when they change and re-use existing Sketches for the rest of the widgets
    in the Scene.
    """

    class FillCall(NamedTuple):
        shape: Shape
        paint: Paint
        opacity: float = 1.

    class StrokeCall(NamedTuple):
        shape: Shape  # maybe it would make sense to add a range to the shape, in case you only want to draw from t1->t2
        paint: Paint
        opacity: float = 1.
        line_width: float = 1.
        cap: nanovg.LineCap = nanovg.LineCap.BUTT
        join: nanovg.LineJoin = nanovg.LineJoin.MITER

    class Hitbox(NamedTuple):
        shape: Shape
        callback: Operator

    draw_calls: List[Union[FillCall, StrokeCall]]
    hitboxes: List[Hitbox]


# PAINTER ##############################################################################################################

class Painter:

    _frame_counter: int = 0
    _frame_start: float = 0

    def __init__(self, window, context):
        self._window = window
        self._context = context
        self._window_size: Size2i = Size2i(*glfw.get_window_size(window))
        self._buffer_size: Size2i = Size2i(*glfw.get_framebuffer_size(window))
        self._hitboxes: List[Sketch.Hitbox] = []

    def __enter__(self) -> Painter:
        if Painter._frame_counter == 0:
            Painter._frame_start = perf_counter()
        Painter._frame_counter += 1

        gl.viewport(0, 0, self._buffer_size.width, self._buffer_size.height)
        gl.clearColor(0.098, 0.098, .098, 1)
        gl.clear(gl.COLOR_BUFFER_BIT, gl.DEPTH_BUFFER_BIT, gl.STENCIL_BUFFER_BIT)

        gl.enable(gl.BLEND)
        gl.blendFunc(gl.SRC_ALPHA, gl.ONE_MINUS_SRC_ALPHA)
        gl.enable(gl.CULL_FACE)
        gl.disable(gl.DEPTH_TEST)

        self._hitboxes.clear()
        nanovg.begin_frame(self._context, self._window_size.width, self._window_size.height,
                           self._buffer_size.width / self._window_size.width)

        return self

    def __exit__(self, exc_type, exc_val, exc_tb):
        nanovg.end_frame(self._context)
        glfw.swap_buffers(self._window)

        frame_end: float = perf_counter()
        if (frame_end - Painter._frame_start) > 1:
            print(f'Drew {Painter._frame_counter} frames last second '
                  f'-> average time {((frame_end - Painter._frame_start) / Painter._frame_counter) * 1000}ms')
            Painter._frame_counter = 0

    def get_hitboxes(self) -> List[Sketch.Hitbox]:
        return self._hitboxes  # TODO: I am not sure that the Painter is the best place to store hitboxes

    def paint(self, node: Node) -> None:
        node_opacity: float = max(0., min(1., float(node.get_property('sys.opacity').get_value())))

        nanovg.reset(self._context)
        nanovg.global_alpha(self._context, node_opacity)
        nanovg.transform(self._context, *node.get_composition().xform)

        sketch: Sketch = node.get_sketch()
        self._hitboxes.extend(sketch.hitboxes)
        for draw_call in sketch.draw_calls:
            if isinstance(draw_call, Sketch.FillCall):
                self._set_shape(draw_call.shape)
                nanovg.fill_paint(self._context, self._create_paint(draw_call.paint))
                nanovg.global_alpha(self._context, max(0., min(1., draw_call.opacity)) * node_opacity)
                nanovg.fill(self._context)

            elif isinstance(draw_call, Sketch.StrokeCall):
                self._set_shape(draw_call.shape)
                nanovg.stroke_paint(self._context, self._create_paint(draw_call.paint))
                nanovg.global_alpha(self._context, max(0., min(1., draw_call.opacity)) * node_opacity)
                nanovg.stroke_width(self._context, draw_call.line_width)
                nanovg.line_cap(self._context, draw_call.cap)
                nanovg.line_join(self._context, draw_call.join)
                nanovg.stroke(self._context)

            else:
                assert False

    def _set_shape(self, shape: Shape):
        nanovg.begin_path(self._context)
        start_point: V2f = shape.get_vertex(0)
        nanovg.move_to(self._context, start_point.x, start_point.y)
        for cubic in shape.iter_outline():
            _, ctrl1, ctrl2, end = cubic.get_vertices()
            nanovg.bezier_to(self._context, ctrl1.x, ctrl1.y, ctrl2.x, ctrl2.y, end.x, end.y)

    def _create_paint(self, paint: Paint) -> nanovg.Paint:
        if isinstance(paint, SolidColor):
            return nanovg.Paint(
                xform=(1, 0, 0, 1, 0, 0),
                extend=(0, 0),
                radius=0,
                feather=1,
                innerColor=paint.color,
                outerColor=paint.color,
                image=0)
        elif isinstance(paint, LinearGradient):
            return nanovg.linear_gradient(self._context, paint.start_point.x, paint.start_point.y, paint.end_point.x,
                                          paint.end_point.y, paint.start_color, paint.end_color)
        elif isinstance(paint, BoxGradient):
            return nanovg.box_gradient(self._context, paint.box.left, paint.box.top, paint.box.width, paint.box.height,
                                       paint.radius, paint.feather, paint.inner_color, paint.outer_color)
        elif isinstance(paint, RadialGradient):
            return nanovg.radial_gradient(self._context, paint.center.x, paint.center.y, paint.inner_radius,
                                          paint.outer_radius, paint.inner_color, paint.outer_color)
        assert False


# DESIGN ###############################################################################################################

class Design:
    """
    A Design is a little static DAG that produces a Sketch when evaluated.

    The idea is that you start with maybe a few expressions that define the first shape.
    You can then use another expression to maybe get the center of the shape that you have just created and then
    use that to create another shape.
    In the end, you have a list of draw calls to the shapes that you have defined.

    The Sketch has access to the properties of its node.
    """

    class _Context(NamedTuple):
        node: Node
        generation: int

    # VALUE BUILDERS ##########################################################

    class _ValueBuilder(ABC):
        @abstractmethod
        def get_schema(self) -> Value.Schema:
            raise NotImplementedError()

        @abstractmethod
        def iter_upstream(self) -> Iterable[Design._ValueBuilder]:
            raise NotImplementedError()

        @abstractmethod
        def evaluate(self, context: Design._Context) -> Value:
            # TODO: this should return a pair<Value, bool> where the bool indicates whether the value is new or not
            raise NotImplementedError()

    class Constant(_ValueBuilder):
        def __init__(self, value: Value):
            """
            The value of this constant.
            """
            self._value: Value = value

        def get_schema(self) -> Value.Schema:
            return self._value.get_schema()

        def iter_upstream(self) -> Iterable[Design._ValueBuilder]:
            return iter(())  # empty generator

        def evaluate(self, _: Design._Context) -> Value:
            return self._value

    class Property(_ValueBuilder):
        def __init__(self, node: Node, name: str):
            """
            :param node: Node from which to get the
            :param name: Name of the Property on the Node.
            """
            self._cached: Value = node.get_property(name).get_value()
            self._generation: int = 0
            self._name: str = name

        def get_schema(self) -> Value.Schema:
            return self._cached.get_schema()

        def iter_upstream(self) -> Iterable[Design._ValueBuilder]:
            return iter(())  # empty generator

        def evaluate(self, context: Design._Context) -> Value:
            if context.generation == self._generation:
                return self._cached

            self._cached = context.node.get_property(self._name).get_value()
            self._generation = context.generation
            return self._cached

    class Expression(_ValueBuilder):
        def __init__(self, schema: Value.Schema, source: str, **arguments: Design._ValueBuilder):
            """
            Source of the Expression.
            The expression has access to all Properties of the Node using `node["property.name"]`.
            """
            assert 'node' not in arguments
            self._cached: Value = Value(schema)
            self._generation: int = 0
            self._expression: Expression = Expression(source)
            self._scope: Dict[str, Design._ValueBuilder] = arguments

        def get_schema(self) -> Value.Schema:
            return self._cached.get_schema()

        def iter_upstream(self) -> Iterable[Design._ValueBuilder]:
            for dependency in self._scope.values():
                yield dependency

        def evaluate(self, context: Design._Context) -> Value:
            if context.generation == self._generation:
                return self._cached

            class _NodeProperties:
                def __getattr__(self, property_name: str) -> Value:
                    if property_name == 'grant':
                        return context.node.get_composition().grant
                    return context.node.get_property(property_name).get_value()

            scope = dict(node=_NodeProperties())
            for name, value_builder in self._scope:
                scope[name] = value_builder.evaluate(context)

            self._cached = Value(self._expression.execute(scope))
            self._generation = context.generation
            return self._cached

    # SHAPE BUILDERS ##########################################################

    class _ShapeBuilder(ABC):
        @abstractmethod
        def iter_upstream(self) -> Iterable[Design._ValueBuilder]:
            raise NotImplementedError()

        @abstractmethod
        def evaluate(self, context: Design._Context) -> Shape:
            raise NotImplementedError()

    class RoundedRect(_ShapeBuilder):
        def __init__(self, x: Design._ValueBuilder, y: Design._ValueBuilder, width: Design._ValueBuilder,
                     height: Design._ValueBuilder, radius: Design._ValueBuilder):
            for arg in (x, y, width, height, radius):
                assert arg.get_schema() == Value(0).get_schema()
            self._x: Design._ValueBuilder = x
            self._y: Design._ValueBuilder = y
            self._width: Design._ValueBuilder = width
            self._height: Design._ValueBuilder = height
            self._radius: Design._ValueBuilder = radius

        def iter_upstream(self) -> Iterable[Design._ValueBuilder]:
            for dependency in (self._x, self._y, self._width, self._height, self._radius):
                yield dependency

        def evaluate(self, context: Design._Context) -> Shape:
            x: float = float(self._x.evaluate(context))
            y: float = float(self._y.evaluate(context))
            width: float = float(self._width.evaluate(context))
            height: float = float(self._height.evaluate(context))
            radius: float = float(self._radius.evaluate(context))
            shape_builder: ShapeBuilder = ShapeBuilder(start=V2f(x + radius, y))
            shape_builder.add_segment(end=V2f(x + width - radius, y))
            shape_builder.add_segment(ctrl1=V2f(x + width - radius + (KAPPA * radius), y),
                                      ctrl2=V2f(x + width, y + radius - (KAPPA * radius)),
                                      end=V2f(x + width, y + radius))
            shape_builder.add_segment(end=V2f(x + width, y + height - radius))
            shape_builder.add_segment(ctrl1=V2f(x + width, y + height - radius + (radius * KAPPA)),
                                      ctrl2=V2f(x + width - radius + (radius * KAPPA), y + height),
                                      end=V2f(x + width - radius, y + height))
            shape_builder.add_segment(end=V2f(x + radius, y + height))
            shape_builder.add_segment(ctrl1=V2f(x + radius - (radius * KAPPA), y + height),
                                      ctrl2=V2f(x, y + height - radius + (radius * KAPPA)),
                                      end=V2f(x, y + height - radius))
            shape_builder.add_segment(end=V2f(x, y + radius))
            shape_builder.add_segment(ctrl1=V2f(x, y + radius - (radius * KAPPA)),
                                      ctrl2=V2f(x + radius - (radius * KAPPA), y),
                                      end=V2f(x + radius, y))
            return Shape(shape_builder)

    # PAINT BUILDERS ##########################################################

    class _PaintBuilder(ABC):
        @abstractmethod
        def iter_upstream(self) -> Iterable[Design._ValueBuilder]:
            raise NotImplementedError()

        @abstractmethod
        def evaluate(self, context: Design._Context) -> Paint:
            raise NotImplementedError()

    class SolidColor(_ShapeBuilder):
        def __init__(self, color: Design._ValueBuilder):
            assert color.get_schema() == Value(r=0, g=0, b=0, a=0).get_schema()
            self._color: Design._ValueBuilder = color

        def iter_upstream(self) -> Iterable[Design._ValueBuilder]:
            yield self._color

        def evaluate(self, context: Design._Context) -> Paint:
            color: Value = self._color.evaluate(context)
            return SolidColor(color=nanovg.Color(
                r=float(color['r']), g=float(color['g']), b=float(color['b']), a=float(color['a'])))

    # DRAW CALL ###############################################################

    class FillCall:
        def __init__(self, *, shape: Design._ShapeBuilder, paint: Design._PaintBuilder,
                     opacity: Optional[Design._ValueBuilder] = None):
            self.shape = shape
            self.paint = paint
            self.opacity = opacity or Design.Constant(Value(1))
            assert self.opacity.get_schema() == Value(0).get_schema()

    class StrokeCall:
        def __init__(self, *, shape: Design._ShapeBuilder, paint: Design._PaintBuilder,
                     opacity: Optional[Design._ValueBuilder] = None,
                     line_width: Optional[Design._ValueBuilder] = None,
                     cap: Optional[Design._ValueBuilder] = None,
                     join: Optional[Design._ValueBuilder] = None):
            self.shape = shape
            self.paint = paint
            self.opacity = opacity or Design.Constant(Value(1))
            self.line_width = line_width or Design.Constant(Value(1))
            self.cap = cap or Design.Constant(Value(0))
            self.join = join or Design.Constant(Value(4))
            assert self.opacity.get_schema() == Value(0).get_schema()
            assert self.line_width.get_schema() == Value(0).get_schema()
            assert self.cap.get_schema() == Value(0).get_schema()
            assert self.join.get_schema() == Value(0).get_schema()

    class HitboxCall(NamedTuple):
        shape: Design._ShapeBuilder
        callback: Operator

    # DESIGN ##################################################################

    def __init__(self, *calls: Union[Design.FillCall, Design.StrokeCall, Design.HitboxCall]):
        self._draw_calls: List[Union[Design.FillCall, Design.StrokeCall]] = []
        self._hitboxes: List[Design.HitboxCall] = []
        self._generation: int = 0

        self._ingest(calls)

    def _ingest(self, calls: Tuple[Union[Design.FillCall, Design.StrokeCall, Design.HitboxCall]]):
        for call in calls:
            if isinstance(call, Design.HitboxCall):
                self._hitboxes.append(call)
            elif isinstance(call, Design.FillCall):
                self._draw_calls.append(call)
            else:
                assert isinstance(call, Design.StrokeCall)
                self._draw_calls.append(call)

    def sketch(self, node: Node) -> Sketch:
        self._generation += 1
        context: Design._Context = Design._Context(node=node, generation=self._generation)
        sketch: Sketch = Sketch(draw_calls=[], hitboxes=[])
        for draw_call in self._draw_calls:
            if isinstance(draw_call, Design.FillCall):
                sketch.draw_calls.append(Sketch.FillCall(
                    shape=draw_call.shape.evaluate(context),
                    paint=draw_call.paint.evaluate(context),
                    opacity=float(draw_call.opacity.evaluate(context)),
                ))
            else:
                assert isinstance(draw_call, Design.StrokeCall)
                # noinspection PyArgumentList
                sketch.draw_calls.append(Sketch.StrokeCall(
                    shape=draw_call.shape.evaluate(context),
                    paint=draw_call.paint.evaluate(context),
                    opacity=float(draw_call.opacity.evaluate(context)),
                    line_width=float(draw_call.line_width.evaluate(context)),
                    cap=nanovg.LineCap(int(draw_call.cap.evaluate(context))),
                    join=nanovg.LineJoin(int(draw_call.join.evaluate(context))),
                ))
        for hitbox in self._hitboxes:
            sketch.hitboxes.append(Sketch.Hitbox(
                shape=hitbox.shape.evaluate(context),
                callback=hitbox.callback,
            ))
        return sketch


from ..main import Node, Operator
