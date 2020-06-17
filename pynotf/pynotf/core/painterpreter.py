from __future__ import annotations

from abc import ABC, abstractmethod
from typing import Dict, List, NamedTuple, Union, Optional, Tuple, Any
from time import perf_counter

import glfw

import pynotf.extra.pynanovg as nanovg
import pynotf.extra.opengl as gl
from pynotf.extra.utils import KAPPA
from pycnotf import V2f, Size2i, Aabrf

from pynotf.data import Value, Shape, ShapeBuilder, RowHandle
import pynotf.core as core


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
        callback: core.Operator

    draw_calls: List[Union[FillCall, StrokeCall]]
    hitboxes: List[Hitbox]


# PAINTER ##############################################################################################################

class Painter:
    _frame_counter: int = 0
    _frame_start: float = 0

    def __init__(self, window, context):
        self._window = window
        self._context = context
        # TODO: glfw.get_window_size and .get_framebuffer_size are extremely slow
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
            # print(f'Drew {Painter._frame_counter} frames last second '
            #       f'-> average time {((frame_end - Painter._frame_start) / Painter._frame_counter) * 1000}ms')
            Painter._frame_counter = 0

    def get_hitboxes(self) -> List[Sketch.Hitbox]:
        return self._hitboxes  # TODO: I am not sure that the Painter is the best place to store hitboxes

    def paint(self, node: core.Node) -> None:
        node_opacity: float = max(0., min(1., float(node.get_interop('widget.opacity').get_value())))

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


class ConstantColor(PaintNode):
    SCHEMA: Value.Schema = Value(r=0, g=0, b=0, a=0).get_schema()

    def __init__(self, color: Value):
        assert color.get_schema() == self.SCHEMA
        self._color: Paint = SolidColor(color=nanovg.Color(
            r=float(color['r']), g=float(color['g']), b=float(color['b']), a=float(color['a'])))

    def evaluate(self, context: Context) -> Tuple[Paint, bool]:
        return self._color, False


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
        assert self.opacity.get_schema().is_number()


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
        assert self.opacity.get_schema().is_number()
        assert self.line_width.get_schema().is_number()
        assert self.cap.get_schema().is_number()
        assert self.join.get_schema().is_number()


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
        self._draw_calls: List[Union[FillCall, StrokeCall]] = []
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
            raise ValueError(f'Cannot build a Design from an incompatible Value Schema:\n{value.get_schema()!s}.\n'
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
                    f'Cannot create a Value Node from an incompatible Value Schema:\n{value_node.get_schema()!s}\n'
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
                        f'Cannot create an Interop Node from an incompatible Value Schema:\n{node_args.get_schema()!s}\n'
                        f'A valid Interop Node Value has the Schema:\n{InteropValue.SCHEMA!s}')
                result = InteropValue(str(node_args))

            # expression value
            elif node_type == 'expression':
                if node_args.get_schema() != ExpressionValue.SCHEMA:
                    raise ValueError(
                        f'Cannot create an Expression Node from an incompatible Value Schema:\n{node_args.get_schema()!s}\n'
                        f'A valid Expression Node Value has the Schema:\n{ExpressionValue.SCHEMA!s}')
                kwargs: Value = node_args['kwargs']
                result = ExpressionValue(
                    source=str(node_args['source']),
                    kwargs={name: parse_value_node(kwargs[name]) for name in kwargs.get_keys()},
                )

            # unknown
            else:
                raise ValueError(f'Unexpected Shape Node type: "{node_type}"')

            # cache the node and return
            values[(node_type, node_args)] = result
            return result

        def parse_shape_node(shape_node: Value) -> ShapeNode:
            # explode value
            if shape_node.get_schema() != ShapeNode.SCHEMA:
                raise ValueError(
                    f'Cannot create a Shape Node from an incompatible Value Schema:\n{shape_node.get_schema()!s}\n'
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
                        f'Cannot create a Rounded Rect from an incompatible Value Schema:\n{node_args.get_schema()!s}\n'
                        f'A valid Rounded Rect Value has the Schema:\n{RoundedRectShape.SCHEMA!s}')
                result = RoundedRectShape(
                    x=parse_value_node(node_args['x']),
                    y=parse_value_node(node_args['y']),
                    width=parse_value_node(node_args['width']),
                    height=parse_value_node(node_args['height']),
                    radius=parse_value_node(node_args['radius']),
                )

            # constant shape
            elif node_type == 'constant':
                if node_args.get_schema() != ConstantShape.SCHEMA:
                    raise ValueError(
                        f'Cannot create a Constant Shape from an incompatible Value Schema:\n{node_args.get_schema()!s}\n'
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
                    f'Cannot create a Paint Node from an incompatible Value Schema:\n{paint_node.get_schema()!s}\n'
                    f'A valid Paint Node Value has the Schema:\n{PaintNode.SCHEMA!s}')
            node_type: str = str(paint_node[0])
            node_args: Value = paint_node[1]

            # re-use existing paint node
            if (node_type, node_args) in paints:
                return paints[(node_type, node_args)]
            result: PaintNode

            # constant color
            if node_type == 'constant':
                if node_args.get_schema() != ConstantColor.SCHEMA:
                    raise ValueError(
                        f'Cannot create a Constant Color from an incompatible Value Schema:\n{node_args.get_schema()!s}\n'
                        f'A valid Constant Color Node Value has the Schema:\n{ConstantColor.SCHEMA!s}')
                result = ConstantColor(node_args)

            # unknown
            else:
                raise ValueError(f'Unexpected Shape Node type: "{node_type}"')

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
                        f'Cannot create a Fill Call from an incompatible Value Schema:\n{call_args.get_schema()!s}\n'
                        f'A valid Fill Call Value has the Schema:\n{FillCall.SCHEMA!s}')
                design._draw_calls.append(FillCall(
                    shape=parse_shape_node(call_args['shape']),
                    paint=parse_paint_node(call_args['paint']),
                    opacity=parse_value_node(call_args['opacity']),
                ))

            elif call_type == "stroke":
                if call_args.get_schema() != StrokeCall.SCHEMA:
                    raise ValueError(
                        f'Cannot create a Stroke Call from an incompatible Value Schema:\n{call_args.get_schema()!s}\n'
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
                raise NotImplementedError("Writing is not implemented yet")

            elif call_type == "mark":
                if call_args.get_schema() != MarkCall.SCHEMA:
                    raise ValueError(
                        f'Cannot create a Mark Call from an incompatible Value Schema:\n{call_args.get_schema()!s}\n'
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
            else:
                assert isinstance(draw_call, StrokeCall)
                for shape in draw_call.shape.evaluate(context)[0]:
                    # noinspection PyArgumentList
                    sketch.draw_calls.append(Sketch.StrokeCall(
                        shape=shape,
                        paint=draw_call.paint.evaluate(context)[0],
                        opacity=float(draw_call.opacity.evaluate(context)[0]),
                        line_width=float(draw_call.line_width.evaluate(context)[0]),
                        cap=nanovg.LineCap(int(draw_call.cap.evaluate(context)[0])),
                        join=nanovg.LineJoin(int(draw_call.join.evaluate(context)[0])),
                    ))
        for hitbox in self._mark_calls:
            assert hitbox.interop.is_valid()
            for shape in hitbox.shape.evaluate(context)[0]:
                sketch.hitboxes.append(Sketch.Hitbox(
                    shape=shape,
                    callback=hitbox.interop,
                ))
        return sketch
