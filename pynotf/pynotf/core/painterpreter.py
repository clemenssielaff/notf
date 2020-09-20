from __future__ import annotations

from abc import ABC, abstractmethod
from typing import Dict, List, NamedTuple, Union, Optional, Tuple, Any
from time import perf_counter

import glfw

import pynotf.extra.opengl as gl
from pynotf.extra.utils import KAPPA
from pycnotf import V2f, Size2i, Aabrf, NanoVG, Color, LineCap, Paint as NvgPaint, Font, Align, M3f
# TODO: Why only LineCap but no LineJoin?
# TODO: Align is nondescript - use TextAlign

from pynotf.data import Value, Shape, ShapeBuilder, RowHandle
import pynotf.core as core

__all__ = ('Painter', 'Sketch', 'Design')


# PAINT ################################################################################################################

class SolidColor(NamedTuple):
    color: Color


class LinearGradient(NamedTuple):
    start_point: V2f
    end_point: V2f
    start_color: Color
    end_color: Color


class BoxGradient(NamedTuple):
    box: Aabrf
    radius: float
    feather: float
    inner_color: Color
    outer_color: Color


class RadialGradient(NamedTuple):
    center: V2f
    inner_radius: float
    outer_radius: float
    inner_color: Color
    outer_color: Color


Paint = Union[SolidColor, LinearGradient, BoxGradient, RadialGradient]


# SKETCH ###############################################################################################################

class Sketch(NamedTuple):
    """
    A Sketch is the result of a Design interpreted by a Painterpreter.
    It is everything that the Painter needs to draw pixels to the screen, but is high-level enough to persist between
    frames.
    Ideally, you would update Sketches only when they change and re-use existing Sketches for the res of the widgets
    in the Scene.
    """

    class FillCall(NamedTuple):
        shape: Shape
        paint: Paint  # TODO: why not store the original NanoVG Paint here instead?
        opacity: float = 1.

    class StrokeCall(NamedTuple):
        shape: Shape  # maybe it would make sense to add a range to the shape, in case you only want to draw from t1->t2
        paint: Paint
        opacity: float = 1.
        line_width: float = 1.
        cap: LineCap = LineCap.BUTT
        join: LineCap = LineCap.MITER

    class WriteCall(NamedTuple):
        pos: V2f
        text: str
        font: str
        size: float
        color: Color = Color(1., 1., 1.)
        blur: float = 0.
        letter_spacing: float = 0.
        line_height: float = 1.
        text_align: Align = Align.LEFT | Align.BASELINE
        width: Optional[float] = None
        # TODO: is it weird that the _drawing_ library takes care of text setting?
        #   * on the one hand, not really - it is basically like drawing anything and the text setting is a "service"
        #   * on the other hand, I could image situations in which we wanted to have finer control over where individual
        #     glyphs are placed
        #   * then again, they are usually placed by taking a string and laying it out according to rules like spacing

    class Hitbox(NamedTuple):  # TODO: the verb "to hitbox" is called "mark" - this is inconsistent and both are meh
        shape: Shape
        callback: core.Operator

    draw_calls: List[Union[FillCall, StrokeCall, WriteCall]]
    hitboxes: List[Hitbox]


# PAINTER ##############################################################################################################

class Painter:
    _frame_counter: int = 0
    _frame_start: float = 0

    def __init__(self, window, nanovg: NanoVG):
        self._window = window
        self._nanovg: NanoVG = nanovg
        # TODO: glfw.get_window_size and .get_framebuffer_size are extremely slow
        self._window_size: Size2i = Size2i(*glfw.get_window_size(window))
        self._buffer_size: Size2i = Size2i(*glfw.get_framebuffer_size(window))
        self._hitboxes: List[Sketch.Hitbox] = []

    def __enter__(self) -> Painter:
        if Painter._frame_counter == 0:
            Painter._frame_start = perf_counter()
        Painter._frame_counter += 1

        gl.viewport(0, 0, self._buffer_size.width, self._buffer_size.height)
        gl.clearColor(0.298, 0.298, .322, 1)
        gl.clear(gl.COLOR_BUFFER_BIT, gl.DEPTH_BUFFER_BIT, gl.STENCIL_BUFFER_BIT)

        gl.enable(gl.BLEND)
        gl.blendFunc(gl.SRC_ALPHA, gl.ONE_MINUS_SRC_ALPHA)
        gl.enable(gl.CULL_FACE)
        gl.disable(gl.DEPTH_TEST)

        self._hitboxes.clear()
        self._nanovg.begin_frame(self._window_size.width, self._window_size.height,
                                 self._buffer_size.width / self._window_size.width)

        return self

    def __exit__(self, exc_type, exc_val, exc_tb):
        self._nanovg.end_frame()
        glfw.swap_buffers(self._window)

        frame_end: float = perf_counter()
        if (frame_end - Painter._frame_start) > 1:
            # print(f'Drew {Painter._frame_counter} frames last second '
            #       f'-> average time {((frame_end - Painter._frame_start) / Painter._frame_counter) * 1000}ms')
            Painter._frame_counter = 0

    def get_hitboxes(self) -> List[Sketch.Hitbox]:
        return self._hitboxes  # TODO: I am not sure that the Painter is the best place to store hitboxes

    def paint(self, node: core.Node) -> None:
        node_opacity: float = max(0., min(1., float(node.get_interop('widget.opacity').get_value())))

        self._nanovg.reset()
        self._nanovg.global_alpha(node_opacity)

        # the offset xform is only used for painting, not for layout
        layout_xform: M3f = M3f(*node.get_composition().xform)
        offset_xform: M3f = M3f(*(float(e) for e in node.get_interop('widget.xform').get_value()))
        self._nanovg.transform(*(layout_xform * offset_xform))

        sketch: Sketch = node.get_sketch()
        self._hitboxes.extend(sketch.hitboxes)
        for draw_call in sketch.draw_calls:
            if isinstance(draw_call, Sketch.FillCall):
                self._set_shape(draw_call.shape)
                self._nanovg.fill_paint(self._create_paint(draw_call.paint))
                self._nanovg.global_alpha(max(0., min(1., draw_call.opacity)) * node_opacity)
                self._nanovg.fill()

            elif isinstance(draw_call, Sketch.StrokeCall):
                self._set_shape(draw_call.shape)
                self._nanovg.stroke_paint(self._create_paint(draw_call.paint))
                self._nanovg.global_alpha(max(0., min(1., draw_call.opacity)) * node_opacity)
                self._nanovg.stroke_width(draw_call.line_width)
                self._nanovg.line_cap(draw_call.cap)
                self._nanovg.line_join(draw_call.join)
                self._nanovg.stroke()

            elif isinstance(draw_call, Sketch.WriteCall):
                font: Optional[Font] = self._nanovg.find_font(draw_call.font)
                if not font:
                    raise ValueError(f'Unknown Font "{draw_call.font}" - please make sure that it has been created.')
                self._nanovg.font_face(font)
                self._nanovg.font_size(draw_call.size)
                self._nanovg.fill_paint(self._nanovg.solid_color(draw_call.color.to_opaque()))
                self._nanovg.global_alpha(max(0., min(1., draw_call.color.a)) * node_opacity)
                self._nanovg.font_blur(draw_call.blur)
                self._nanovg.text_letter_spacing(draw_call.letter_spacing)
                self._nanovg.text_line_height(draw_call.line_height)
                self._nanovg.text_align(draw_call.text_align)
                if draw_call.width:
                    # TODO: `width` could be an optional argument to `text`
                    self._nanovg.text_box(draw_call.pos.x, draw_call.pos.y, draw_call.text, draw_call.width)
                else:
                    self._nanovg.text(draw_call.pos.x, draw_call.pos.y, draw_call.text)

            else:
                assert False

    def _set_shape(self, shape: Shape):
        self._nanovg.begin_path()
        start_point: V2f = shape.get_vertex(0)
        self._nanovg.move_to(start_point.x, start_point.y)
        for cubic in shape.iter_outline():
            _, ctrl1, ctrl2, end = cubic.get_vertices()
            self._nanovg.bezier_to(ctrl1.x, ctrl1.y, ctrl2.x, ctrl2.y, end.x, end.y)

    def _create_paint(self, paint: Paint) -> NvgPaint:
        if isinstance(paint, SolidColor):
            return self._nanovg.solid_color(paint.color)

        elif isinstance(paint, LinearGradient):
            return self._nanovg.linear_gradient(
                paint.start_point.x, paint.start_point.y,
                paint.end_point.x, paint.end_point.y, paint.start_color, paint.end_color)

        elif isinstance(paint, BoxGradient):
            return self._nanovg.box_gradient(
                paint.box.left, paint.box.height - paint.box.top, paint.box.width,
                paint.box.height, paint.radius, paint.feather, paint.inner_color, paint.outer_color)

        elif isinstance(paint, RadialGradient):
            return self._nanovg.radial_gradient(
                paint.center.x, paint.center.y, paint.inner_radius,
                paint.outer_radius, paint.inner_color, paint.outer_color)

        assert False


# CONTEXT ##############################################################################################################

class Context(NamedTuple):
    node: core.Node
    generation: int


# VALUE NODE ###########################################################################################################

class ValueNode(ABC):
    SCHEMA: Value.Schema = Value.Schema.create(("", Value()))  # [(identifier, arguments)]

    @abstractmethod
    def get_schema(self) -> Value.Schema:
        raise NotImplementedError()

    @abstractmethod
    def evaluate(self, context: Context) -> Tuple[Value, bool]:
        """
        Returns a pair of the Value and whether it is a new value (True) or the same as the last time this method
        was called (False).
        """
        raise NotImplementedError()


class ConstantValue(ValueNode):
    def __init__(self, value: Value):
        """
        The value of this constant.
        """
        self._value: Value = value

    def get_schema(self) -> Value.Schema:
        return self._value.get_schema()

    def evaluate(self, _: Context) -> Tuple[Value, bool]:
        return self._value, False


class InteropValue(ValueNode):
    SCHEMA: Value.Schema = Value("").get_schema()

    def __init__(self, name: str):
        """
        :param name: Name of the InteropValue on the Node.
        """
        self._cached: Value = Value()
        self._generation: int = 0
        self._name: str = name

    def get_schema(self) -> Value.Schema:
        return self._cached.get_schema()

    def evaluate(self, context: Context) -> Tuple[Value, bool]:
        first_run: bool = self._generation == 0
        self._generation = context.generation
        if first_run:
            self._cached = context.node.get_interop(self._name).get_value()
            return self._cached, True
        elif context.generation == self._generation:
            return self._cached, False

        new_value: Value = context.node.get_interop(self._name).get_value()
        if new_value == self._cached:
            return self._cached, False

        self._cached = new_value
        return self._cached, True


class ExpressionValue(ValueNode):
    SCHEMA: Value.Schema = Value(
        source="",
        kwargs=Value(),
    ).get_schema()

    def __init__(self, source: str, kwargs: Dict[str, ValueNode]):
        """
        Source of the ExpressionValue.
        The expression has access to all Interops of the core.Node using `node["interop.name"]`.
        """
        assert 'node' not in kwargs
        self._cached: Value = Value()
        self._generation: int = 0
        self._expression: core.Expression = core.Expression(source)
        self._scope: Dict[str, ValueNode] = kwargs

    def get_schema(self) -> Value.Schema:
        return self._cached.get_schema()

    def evaluate(self, context: Context) -> Tuple[Value, bool]:
        if context.generation == self._generation:
            return self._cached, False
        self._generation = context.generation

        class _NodeInterops:
            def __getattr__(self, interop_name: str) -> Value:
                if interop_name == 'grant':
                    return context.node.get_composition().grant
                return context.node.get_interop(interop_name).get_value()

        scope = dict(node=_NodeInterops())
        for name, value_node in self._scope:
            scope[name] = value_node.evaluate(context)
        result: Any = self._expression.execute(scope)
        new_value: Value
        if isinstance(result, Value):
            new_value = result
        else:
            new_value = Value(result)
        if new_value == self._cached:
            return self._cached, False

        self._cached = new_value
        return self._cached, True


# SHAPE NODE ###########################################################################################################

class ShapeNode(ABC):
    SCHEMA: Value.Schema = Value.Schema.create(("", Value()))  # [(identifier, arguments)]

    @abstractmethod
    def evaluate(self, context: Context) -> Tuple[List[Shape], bool]:
        raise NotImplementedError()


class RoundedRectShape(ShapeNode):
    SCHEMA: Value.Schema = Value(
        x=("", Value()),
        y=("", Value()),
        width=("", Value()),
        height=("", Value()),
        radius=("", Value()),
    ).get_schema()

    def __init__(self,
                 x: ValueNode,
                 y: ValueNode,
                 width: ValueNode,
                 height: ValueNode,
                 radius: ValueNode):
        super().__init__()

        self._cached: List[Shape] = []
        self._generation: int = 0

        self._x: ValueNode = x
        self._y: ValueNode = y
        self._width: ValueNode = width
        self._height: ValueNode = height
        self._radius: ValueNode = radius

    def evaluate(self, context: Context) -> Tuple[List[Shape], bool]:
        if context.generation == self._generation:
            return self._cached, False
        self._generation = context.generation

        # TODO: maybe some kind of push-dirtying would be better?
        #   Of course, that won't easily work with Expressions. Of course, if you separate interface ops out of the
        #   expression and let the expression take interops as inputs, you could push dirtiness up from the
        #   interops ... that might be especially good since I don't want to have to re-evaluate expressions all
        #   the time, just to see that they are equal to the last computed value.
        is_any_new: bool = False
        x_value, is_new = self._x.evaluate(context)
        is_any_new |= is_new
        y_value, is_new = self._y.evaluate(context)
        is_any_new |= is_new
        width_value, is_new = self._width.evaluate(context)
        is_any_new |= is_new
        height_value, is_new = self._height.evaluate(context)
        is_any_new |= is_new
        radius_value, is_new = self._radius.evaluate(context)
        is_any_new |= is_new
        if not is_any_new and self._cached:
            return self._cached, False

        x: float = float(x_value)
        y: float = float(y_value)
        width: float = float(width_value)
        height: float = float(height_value)
        radius: float = float(radius_value)
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
        self._cached = [Shape(shape_builder)]
        return self._cached, True


class LinesShape(ShapeNode):
    SCHEMA: Value.Schema = Value([("", Value())]).get_schema()

    def __init__(self, lines: List[ValueNode]):
        super().__init__()

        self._cached: List[Shape] = []
        self._generation: int = 0

        self._vertices: List[ValueNode] = lines

    def evaluate(self, context: Context) -> Tuple[List[Shape], bool]:
        if context.generation == self._generation:
            return self._cached, False
        self._generation = context.generation

        is_any_new: bool = False
        vertex_values: List[V2f] = []
        for vertex in self._vertices:
            vertex_value, is_new = vertex.evaluate(context)
            vertex_values.append(V2f(float(vertex_value['x']), float(vertex_value['y'])))
            is_any_new |= is_new
        if not is_any_new or len(self._vertices) < 2:
            return self._cached, False

        shape_builder: ShapeBuilder = ShapeBuilder(start=vertex_values[0])
        for vertex_value in vertex_values[1:]:
            shape_builder.add_segment(end=vertex_value)
        self._cached = [Shape(shape_builder)]
        return self._cached, True


class ConstantShape(ShapeNode):
    SCHEMA: Value.Schema = Value([Value(Shape.SCHEMA)]).get_schema()

    def __init__(self, shapes: List[Shape]):
        self._shapes = shapes

    def evaluate(self, _: Context) -> Tuple[List[Shape], bool]:
        return self._shapes, False


# PAINT NODE ###########################################################################################################

class PaintNode(ABC):
    SCHEMA: Value.Schema = Value.Schema.create(("", Value()))  # [(identifier, arguments)]

    @abstractmethod
    def evaluate(self, context: Context) -> Tuple[Paint, bool]:
        raise NotImplementedError()


class SolidColorNode(PaintNode):
    SCHEMA: Value.Schema = Value(r=0, g=0, b=0, a=0).get_schema()

    def __init__(self, color: Value):
        assert color.get_schema() == self.SCHEMA
        self._paint: Paint = SolidColor(color=Color(
            r=float(color['r']), g=float(color['g']), b=float(color['b']), a=float(color['a'])))

    def evaluate(self, context: Context) -> Tuple[Paint, bool]:
        return self._paint, False


class LinearGradientNode(PaintNode):
    SCHEMA: Value.Schema = Value.Schema.create(dict(
        start_point=("", Value()),
        end_point=("", Value()),
        start_color=("", Value()),
        end_color=("", Value()),
    ))

    def __init__(self,
                 start_point: ValueNode,
                 end_point: ValueNode,
                 start_color: ValueNode,
                 end_color: ValueNode):
        super().__init__()

        self._cached: Optional[Paint] = None
        self._generation: int = 0

        self._start_point: ValueNode = start_point
        self._end_point: ValueNode = end_point
        self._start_color: ValueNode = start_color
        self._end_color: ValueNode = end_color

    def evaluate(self, context: Context) -> Tuple[Paint, bool]:
        if context.generation == self._generation:
            return self._cached, False
        self._generation = context.generation

        is_any_new: bool = False
        start_point_value, is_new = self._start_point.evaluate(context)
        is_any_new |= is_new
        end_point_value, is_new = self._end_point.evaluate(context)
        is_any_new |= is_new
        start_color_value, is_new = self._start_color.evaluate(context)
        is_any_new |= is_new
        end_color_value, is_new = self._end_color.evaluate(context)
        is_any_new |= is_new
        if not is_any_new and self._cached:
            return self._cached, False

        self._cached = LinearGradient(
            start_point=V2f(float(start_point_value['x']), float(start_point_value['y'])),
            end_point=V2f(float(end_point_value['x']), float(end_point_value['y'])),
            start_color=Color(r=float(start_color_value['r']), g=float(start_color_value['g']),
                              b=float(start_color_value['b']), a=float(start_color_value['a'])),
            end_color=Color(r=float(end_color_value['r']), g=float(end_color_value['g']),
                            b=float(end_color_value['b']), a=float(end_color_value['a'])),
        )
        return self._cached, True


class BoxGradientNode(PaintNode):
    SCHEMA: Value.Schema = Value.Schema.create(dict(
        box=("", Value()),
        radius=("", Value()),
        feather=("", Value()),
        inner_color=("", Value()),
        outer_color=("", Value()),
    ))

    def __init__(self,
                 box: ValueNode,
                 radius: ValueNode,
                 feather: ValueNode,
                 inner_color: ValueNode,
                 outer_color: ValueNode):
        super().__init__()

        self._cached: Optional[Paint] = None
        self._generation: int = 0

        self._box: ValueNode = box
        self._radius: ValueNode = radius
        self._feather: ValueNode = feather
        self._inner_color: ValueNode = inner_color
        self._outer_color: ValueNode = outer_color

    def evaluate(self, context: Context) -> Tuple[Paint, bool]:
        if context.generation == self._generation:
            return self._cached, False
        self._generation = context.generation

        is_any_new: bool = False
        box_value, is_new = self._box.evaluate(context)
        is_any_new |= is_new
        radius_value, is_new = self._radius.evaluate(context)
        is_any_new |= is_new
        feather_value, is_new = self._feather.evaluate(context)
        is_any_new |= is_new
        inner_color_value, is_new = self._inner_color.evaluate(context)
        is_any_new |= is_new
        outer_color_value, is_new = self._outer_color.evaluate(context)
        is_any_new |= is_new
        if not is_any_new and self._cached:
            return self._cached, False

        self._cached = BoxGradient(
            box=Aabrf(float(box_value['x']), float(box_value['y']),
                      float(box_value['w']), float(box_value['h'])),
            radius=float(radius_value),
            feather=float(feather_value),
            inner_color=Color(r=float(inner_color_value['r']), g=float(inner_color_value['g']),
                              b=float(inner_color_value['b']), a=float(inner_color_value['a'])),
            outer_color=Color(r=float(outer_color_value['r']), g=float(outer_color_value['g']),
                              b=float(outer_color_value['b']), a=float(outer_color_value['a'])),
        )
        return self._cached, True


class RadialGradientNode(PaintNode):
    SCHEMA: Value.Schema = Value.Schema.create(dict(
        center=("", Value()),
        inner_radius=("", Value()),
        outer_radius=("", Value()),
        inner_color=("", Value()),
        outer_color=("", Value()),
    ))

    def __init__(self,
                 center: ValueNode,
                 inner_radius: ValueNode,
                 outer_radius: ValueNode,
                 inner_color: ValueNode,
                 outer_color: ValueNode):
        super().__init__()

        self._cached: Optional[Paint] = None
        self._generation: int = 0

        self._center: ValueNode = center
        self._inner_radius: ValueNode = inner_radius
        self._outer_radius: ValueNode = outer_radius
        self._inner_color: ValueNode = inner_color
        self._outer_color: ValueNode = outer_color

    def evaluate(self, context: Context) -> Tuple[Paint, bool]:
        if context.generation == self._generation:
            return self._cached, False
        self._generation = context.generation

        is_any_new: bool = False
        center_value, is_new = self._center.evaluate(context)
        is_any_new |= is_new
        inner_radius_value, is_new = self._inner_radius.evaluate(context)
        is_any_new |= is_new
        outer_radius_value, is_new = self._outer_radius.evaluate(context)
        is_any_new |= is_new
        inner_color_value, is_new = self._inner_color.evaluate(context)
        is_any_new |= is_new
        outer_color_value, is_new = self._outer_color.evaluate(context)
        is_any_new |= is_new
        if not is_any_new and self._cached:
            return self._cached, False

        self._cached = RadialGradient(
            center=V2f(float(center_value['x']), float(center_value['y'])),
            inner_radius=float(inner_radius_value),
            outer_radius=float(outer_radius_value),
            inner_color=Color(r=float(inner_color_value['r']), g=float(inner_color_value['g']),
                              b=float(inner_color_value['b']), a=float(inner_color_value['a'])),
            outer_color=Color(r=float(outer_color_value['r']), g=float(outer_color_value['g']),
                              b=float(outer_color_value['b']), a=float(outer_color_value['a'])),
        )
        return self._cached, True


# DRAW CALLS ###########################################################################################################

class FillCall:
    SCHEMA: Value.Schema = Value(
        shape=("", Value()),
        paint=("", Value()),
        opacity=("", Value()),
    ).get_schema()

    def __init__(self, *,
                 shape: ShapeNode,
                 paint: PaintNode,
                 opacity: Optional[ValueNode] = None):
        self.shape = shape
        self.paint = paint
        self.opacity = opacity or ConstantValue(Value(1))


class StrokeCall:
    SCHEMA: Value.Schema = Value(
        shape=("", Value()),
        paint=("", Value()),
        opacity=("", Value()),
        line_width=("", Value()),
        cap=("", Value()),
        join=("", Value()),
    ).get_schema()

    # TODO: Schema.is_subset to test whether a Schema is a subset of another
    #   This way, we can implement "optional" arguments by passing in a Value that contains some but not all fields.
    #   I suppose this is only relevant for records.

    def __init__(self, *,
                 shape: ShapeNode,
                 paint: PaintNode,
                 opacity: Optional[ValueNode] = None,
                 line_width: Optional[ValueNode] = None,
                 cap: Optional[ValueNode] = None,
                 join: Optional[ValueNode] = None):
        self.shape = shape
        self.paint = paint
        self.opacity = opacity or ConstantValue(Value(1))
        self.line_width = line_width or ConstantValue(Value(1))
        self.cap = cap or ConstantValue(Value(0))
        self.join = join or ConstantValue(Value(4))


class WriteCall:
    SCHEMA: Value.Schema = Value(
        pos=("", Value()),
        text=("", Value()),
        font=("", Value()),
        size=("", Value()),
        color=("", Value()),
        blur=("", Value()),
        letter_spacing=("", Value()),
        line_height=("", Value()),
        text_align=("", Value()),
        width=("", Value()),
    ).get_schema()

    def __init__(self, *,
                 pos: ValueNode,
                 text: ValueNode,
                 font: ValueNode,
                 size: ValueNode,
                 color: Optional[ValueNode] = None,
                 blur: Optional[ValueNode] = None,
                 letter_spacing: Optional[ValueNode] = None,
                 line_height: Optional[ValueNode] = None,
                 text_align: Optional[ValueNode] = None,
                 width: Optional[ValueNode] = None):
        self.pos = pos
        self.text = text
        self.font = font
        self.size = size
        self.color = color or ConstantValue(Value(r=1, g=1, b=1, a=1))
        self.blur = blur or ConstantValue(Value(0))
        self.letter_spacing = letter_spacing or ConstantValue(Value(0))
        self.line_height = line_height or ConstantValue(Value(1))
        self.text_align = text_align or ConstantValue(Value(65))  # Align.LEFT | Align.BASELINE
        self.width = width or ConstantValue(Value())


class MarkCall:
    SCHEMA: Value.Schema = Value(
        shape=("", Value()),
        interop="",
    ).get_schema()

    def __init__(self, shape: ShapeNode, interop: str):
        self.shape: ShapeNode = shape
        self.interop_name: str = interop
        self.interop: core.Operator = core.Operator(RowHandle())

    def initialize(self, node: core.Node) -> None:
        self.interop: core.Operator = node.get_interop(self.interop_name)
        assert self.interop and self.interop.get_input_schema() == Value(x=0, y=0).get_schema()


# DESIGN ###############################################################################################################

class Design:
    """
    A Design is a little static DAG that produces a Sketch when evaluated.

    The idea is that you start with maybe a few expressions that define the first shape.
    You can then use another expression to maybe get the center of the shape that you have just created and then
    use that to create another shape.
    In the end, you have a list of draw calls to the shapes that you have defined.

    The Sketch has access to the interops of its node.
    """

    SCHEMA: Value.Schema = Value([("", Value())]).get_schema()  # [(identifier, arguments)]

    def __init__(self):
        self._values: List[ValueNode] = []
        self._shapes: List[ShapeNode] = []
        self._paints: List[PaintNode] = []
        self._draw_calls: List[Union[FillCall, StrokeCall, WriteCall]] = []
        self._mark_calls: List[MarkCall] = []
        self._generation: int = 0

    def initialize(self, node: core.Node):
        for mark in self._mark_calls:
            mark.initialize(node)

    @classmethod
    def from_value(cls, value: Value) -> Design:
        """
        Builds a Design instance from a compatible Value.
        """
        if value.is_none():
            return Design()

        if value.get_schema() != cls.SCHEMA:
            raise ValueError(f'Cannot build a Design from an incompatible Value:\n{value.get_schema()!s}.\n'
                             f'A valid Design Value has the Schema:\n{cls.SCHEMA!s}')

        design: Design = Design()

        # in order to minimize the number of nodes in a Design DAG, reuse those that you already have
        values: Dict[(str, Value), ValueNode] = {}
        shapes: Dict[(str, Value), ShapeNode] = {}
        paints: Dict[(str, Value), PaintNode] = {}

        def parse_value_node(value_node: Value) -> ValueNode:
            # explode value
            if value_node.get_schema() != ValueNode.SCHEMA:
                raise ValueError(
                    f'Cannot create a Value Node from an incompatible Value:\n{value_node.get_schema()!s}\n'
                    f'A valid Value Node Value has the Schema:\n{ValueNode.SCHEMA!s}')
            node_type: str = str(value_node[0])
            node_args: Value = value_node[1]

            # re-use existing value node
            if (node_type, node_args) in shapes:
                return values[(node_type, node_args)]
            result: ValueNode

            # constant value
            if node_type == 'constant':
                result = ConstantValue(node_args)

            # interop value
            elif node_type == 'interop':
                if node_args.get_schema() != InteropValue.SCHEMA:
                    raise ValueError(
                        f'Cannot create an Interop Node from an incompatible Value:\n{node_args.get_schema()!s}\n'
                        f'A valid Interop Node Value has the Schema:\n{InteropValue.SCHEMA!s}')
                result = InteropValue(str(node_args))

            # expression value
            elif node_type == 'expression':
                if node_args.get_schema() != ExpressionValue.SCHEMA:
                    raise ValueError(
                        f'Cannot create an Expression Node from an incompatible Value:\n{node_args.get_schema()!s}\n'
                        f'A valid Expression Node Value has the Schema:\n{ExpressionValue.SCHEMA!s}')
                kwargs: Value = node_args['kwargs']
                result = ExpressionValue(
                    source=str(node_args['source']),
                    kwargs={name: parse_value_node(kwargs[name]) for name in kwargs.get_keys()},
                )

            # unknown
            else:
                raise ValueError(f'Unexpected Value Node type: "{node_type}"')

            # cache the node and return
            values[(node_type, node_args)] = result
            return result

        def parse_shape_node(shape_node: Value) -> ShapeNode:
            # explode value
            if shape_node.get_schema() != ShapeNode.SCHEMA:
                raise ValueError(
                    f'Cannot create a Shape Node from an incompatible Value:\n{shape_node.get_schema()!s}\n'
                    f'A valid Shape Node Value has the Schema:\n{ShapeNode.SCHEMA!s}')
            node_type: str = str(shape_node[0])
            node_args: Value = shape_node[1]

            # re-use existing shape node
            if (node_type, node_args) in shapes:
                return shapes[(node_type, node_args)]
            result: ShapeNode

            # rounded rect
            if node_type == 'rounded_rect':
                if node_args.get_schema() != RoundedRectShape.SCHEMA:
                    raise ValueError(
                        f'Cannot create a Rounded Rect from an incompatible Value:\n{node_args.get_schema()!s}\n'
                        f'A valid Rounded Rect Value has the Schema:\n{RoundedRectShape.SCHEMA!s}')
                result = RoundedRectShape(
                    x=parse_value_node(node_args['x']),
                    y=parse_value_node(node_args['y']),
                    width=parse_value_node(node_args['width']),
                    height=parse_value_node(node_args['height']),
                    radius=parse_value_node(node_args['radius']),
                )

            # line/s
            elif node_type == "lines":
                if node_args.get_schema() != LinesShape.SCHEMA:
                    raise ValueError(
                        f'Cannot create Lines from an incompatible Value:\n{node_args.get_schema()!s}\n'
                        f'A valid Lines Value has the Schema:\n{LinesShape.SCHEMA!s}')
                result = LinesShape([parse_value_node(node_args[i]) for i in range(len(node_args))])

            # constant shape
            elif node_type == 'constant':
                if node_args.get_schema() != ConstantShape.SCHEMA:
                    raise ValueError(
                        f'Cannot create a Constant Shape from an incompatible Value:\n{node_args.get_schema()!s}\n'
                        f'A valid Constant Shape Node Value has the Schema:\n{ConstantShape.SCHEMA!s}')
                result = ConstantShape(shapes=[Shape.from_value(v) for v in node_args])

            # unknown
            else:
                raise ValueError(f'Unexpected Shape Node type: "{node_type}"')

            # cache the node and return
            shapes[(node_type, node_args)] = result
            return result

        def parse_paint_node(paint_node: Value) -> PaintNode:
            # explode value
            if paint_node.get_schema() != PaintNode.SCHEMA:
                raise ValueError(
                    f'Cannot create a Paint Node from an incompatible Value:\n{paint_node.get_schema()!s}\n'
                    f'A valid Paint Node Value has the Schema:\n{PaintNode.SCHEMA!s}')
            node_type: str = str(paint_node[0])
            node_args: Value = paint_node[1]

            # re-use existing paint node
            if (node_type, node_args) in paints:
                return paints[(node_type, node_args)]
            result: PaintNode

            # solid color
            if node_type == 'constant':
                if node_args.get_schema() != SolidColorNode.SCHEMA:
                    raise ValueError(
                        f'Cannot create a Solid Color from an incompatible Value:\n{node_args.get_schema()!s}\n'
                        f'A valid Constant Color Value has the Schema:\n{SolidColorNode.SCHEMA!s}')
                result = SolidColorNode(node_args)

            # linear gradient
            elif node_type == 'linear_gradient':
                if node_args.get_schema() != LinearGradientNode.SCHEMA:
                    raise ValueError(
                        f'Cannot create a Linear Gradient from an incompatible Value:\n{node_args.get_schema()!s}\n'
                        f'A valid Linear Gradient Value has the Schema:\n{LinearGradientNode.SCHEMA!s}')
                result = LinearGradientNode(
                    start_point=parse_value_node(node_args['start_point']),
                    end_point=parse_value_node(node_args['end_point']),
                    start_color=parse_value_node(node_args['start_color']),
                    end_color=parse_value_node(node_args['end_color']),
                )

            # box gradient
            elif node_type == 'box_gradient':
                if node_args.get_schema() != BoxGradientNode.SCHEMA:
                    raise ValueError(
                        f'Cannot create a Box Gradient from an incompatible Value:\n{node_args.get_schema()!s}\n'
                        f'A valid Box Gradient Value has the Schema:\n{BoxGradientNode.SCHEMA!s}')
                result = BoxGradientNode(
                    box=parse_value_node(node_args['box']),
                    radius=parse_value_node(node_args['radius']),
                    feather=parse_value_node(node_args['feather']),
                    inner_color=parse_value_node(node_args['inner_color']),
                    outer_color=parse_value_node(node_args['outer_color']),
                )

            # radial gradient
            elif node_type == 'radial_gradient':
                if node_args.get_schema() != RadialGradientNode.SCHEMA:
                    raise ValueError(
                        f'Cannot create a Radial Gradient from an incompatible Value:\n{node_args.get_schema()!s}\n'
                        f'A valid Radial Gradient Value has the Schema:\n{RadialGradientNode.SCHEMA!s}')
                result = RadialGradientNode(
                    center=parse_value_node(node_args['center']),
                    inner_radius=parse_value_node(node_args['inner_radius']),
                    outer_radius=parse_value_node(node_args['outer_radius']),
                    inner_color=parse_value_node(node_args['inner_color']),
                    outer_color=parse_value_node(node_args['outer_color']),
                )

            # unknown
            else:
                raise ValueError(f'Unexpected Paint Node type: "{node_type}"')

            # cache the node and return
            paints[(node_type, node_args)] = result
            return result

        # parse all calls
        for call in value:
            call_type: str = str(call[0])
            call_args: Value = call[1]

            # top level calls must be one of fill, stroke, write or mark
            if call_type == "fill":
                if call_args.get_schema() != FillCall.SCHEMA:
                    raise ValueError(
                        f'Cannot create a Fill Call from an incompatible Value:\n{call_args.get_schema()!s}\n'
                        f'A valid Fill Call Value has the Schema:\n{FillCall.SCHEMA!s}')
                design._draw_calls.append(FillCall(
                    shape=parse_shape_node(call_args['shape']),
                    paint=parse_paint_node(call_args['paint']),
                    opacity=parse_value_node(call_args['opacity']),
                ))

            elif call_type == "stroke":
                if call_args.get_schema() != StrokeCall.SCHEMA:
                    raise ValueError(
                        f'Cannot create a Stroke Call from an incompatible Value:\n{call_args.get_schema()!s}\n'
                        f'A valid Stroke Call Value has the Schema:\n{StrokeCall.SCHEMA!s}')
                design._draw_calls.append(StrokeCall(
                    shape=parse_shape_node(call_args['shape']),
                    paint=parse_paint_node(call_args['paint']),
                    opacity=parse_value_node(call_args['opacity']),
                    line_width=parse_value_node(call_args['line_width']),
                    cap=parse_value_node(call_args['cap']),
                    join=parse_value_node(call_args['join']),
                ))

            elif call_type == "write":
                if call_args.get_schema() != WriteCall.SCHEMA:
                    raise ValueError(
                        f'Cannot create a Write Call from an incompatible Value:\n{call_args.get_schema()!s}\n'
                        f'A valid Write Call Value has the Schema:\n{WriteCall.SCHEMA!s}')
                design._draw_calls.append(WriteCall(
                    pos=parse_value_node(call_args['pos']),
                    text=parse_value_node(call_args['text']),
                    font=parse_value_node(call_args['font']),
                    size=parse_value_node(call_args['size']),
                    color=parse_value_node(call_args['color']),
                    blur=parse_value_node(call_args['blur']),
                    letter_spacing=parse_value_node(call_args['letter_spacing']),
                    line_height=parse_value_node(call_args['line_height']),
                    text_align=parse_value_node(call_args['text_align']),
                    width=None if call_args['width'].is_none() else parse_value_node(call_args['width']),
                ))

            elif call_type == "mark":
                if call_args.get_schema() != MarkCall.SCHEMA:
                    raise ValueError(
                        f'Cannot create a Mark Call from an incompatible Value:\n{call_args.get_schema()!s}\n'
                        f'A valid Mark Call Value has the Schema:\n{MarkCall.SCHEMA!s}')
                design._mark_calls.append(MarkCall(
                    shape=parse_shape_node(call_args['shape']),
                    interop=str(call_args['interop']),
                ))

            else:
                raise ValueError(f'Unexpected call type: "{call_type}"')

        design._values = list(values.values())
        design._shapes = list(shapes.values())
        design._paints = list(paints.values())

        return design

    def sketch(self, node: core.Node) -> Sketch:
        self._generation += 1
        context: Context = Context(node=node, generation=self._generation)
        sketch: Sketch = Sketch(draw_calls=[], hitboxes=[])
        for draw_call in self._draw_calls:
            if isinstance(draw_call, FillCall):
                for shape in draw_call.shape.evaluate(context)[0]:
                    sketch.draw_calls.append(Sketch.FillCall(
                        shape=shape,
                        paint=draw_call.paint.evaluate(context)[0],
                        opacity=float(draw_call.opacity.evaluate(context)[0]),
                    ))
            elif isinstance(draw_call, StrokeCall):
                for shape in draw_call.shape.evaluate(context)[0]:
                    sketch.draw_calls.append(Sketch.StrokeCall(
                        shape=shape,
                        paint=draw_call.paint.evaluate(context)[0],
                        opacity=float(draw_call.opacity.evaluate(context)[0]),
                        line_width=float(draw_call.line_width.evaluate(context)[0]),
                        cap=LineCap(int(draw_call.cap.evaluate(context)[0])),
                        join=LineCap(int(draw_call.join.evaluate(context)[0])),
                    ))
            else:
                assert isinstance(draw_call, WriteCall)
                pos_value: Value = draw_call.pos.evaluate(context)[0]
                color_value: Value = draw_call.color.evaluate(context)[0]
                width_value: Value = draw_call.width.evaluate(context)[0]
                sketch.draw_calls.append(Sketch.WriteCall(
                    pos=V2f(float(pos_value['x']), float(pos_value['y'])),
                    text=str(draw_call.text.evaluate(context)[0]),
                    font=str(draw_call.font.evaluate(context)[0]),
                    size=float(draw_call.size.evaluate(context)[0]),
                    color=Color(float(color_value['r']), float(color_value['g']),
                                float(color_value['b']), float(color_value['a']), ),
                    blur=float(draw_call.blur.evaluate(context)[0]),
                    letter_spacing=float(draw_call.letter_spacing.evaluate(context)[0]),
                    line_height=float(draw_call.line_height.evaluate(context)[0]),
                    text_align=Align(int(draw_call.text_align.evaluate(context)[0])),
                    width=None if width_value.is_none() else float(width_value),
                ))

        for hitbox in self._mark_calls:
            assert hitbox.interop.is_valid()
            for shape in hitbox.shape.evaluate(context)[0]:
                sketch.hitboxes.append(Sketch.Hitbox(
                    shape=shape,
                    callback=hitbox.interop,
                ))
        return sketch
