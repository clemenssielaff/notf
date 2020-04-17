from enum import IntEnum, IntFlag, unique
from ctypes import CDLL, Structure, c_void_p, c_float, c_int32, c_ubyte, c_char_p, byref
from copy import copy
from functools import wraps
from pathlib import Path


def _load_nanovg():
    library = CDLL(str((Path(__file__).parent / Path("../../lib/libnanovg.so")).resolve()))
    return library


_nanovg = _load_nanovg()


def _c_api_wrapper(name: str, argtypes: [], restype=None):
    """
    Decorator around a function that forwards to a function in the nanovg C API.
    If no function with the given name is found in the api, this decorator returns a function that simply prints an
    error message instead.

    :param name:        Name of the function to call and modified.
    :param argtypes:    Python ctypes argument types of the C function.
    :param restype:     Python ctypes return kind of the C function or None if the return kind is void.
    :return:            Decorated function.
    """
    if hasattr(_nanovg, name):
        def decorator(function):
            c_function = getattr(_nanovg, name)
            c_function.argtypes = argtypes
            c_function.restype = restype

            @wraps(function)
            def wrapper(*args, **kwargs):
                return function(*args, **kwargs)

            return wrapper
    else:
        def decorator(function):
            @wraps(function)
            def wrapper(*_, **__):
                raise LookupError("Missing function '{}' in nanovg library".format(name))

            return wrapper
    return decorator


class Color(Structure):
    _fields_ = [
        ("r", c_float),
        ("g", c_float),
        ("b", c_float),
        ("a", c_float),
    ]


class Paint(Structure):
    _fields_ = [
        ("xform", c_float * 6),
        ("extent", c_float * 2),
        ("radius", c_float),
        ("feather", c_float),
        ("innerColor", Color),
        ("outerColor", Color),
        ("image", c_int32),
    ]


Xform = (c_float * 6)


class GlyphPosition(Structure):
    _fields_ = [
        ("str", c_void_p),  # Position of the glyph in the input string.
        ("x", c_float),  # The x-coordinate of the logical glyph position.
        ("minx", c_float),  # The bounds of the glyph shape.
        ("maxx", c_float),
    ]


class TextRow(Structure):
    _fields_ = [
        # Pointer to the input text where the row starts.
        ("start", c_void_p),
        # Pointer to the input text where the row ends (one past the last character).
        ("end", c_void_p),
        # Pointer to the beginning of the next row.
        ("next", c_void_p),
        # Logical width of the row.
        ("width", c_float),
        # Actual bounds of the row. Logical with and bounds can differ because of kerning and some parts over extending.
        ("minx", c_float),
        ("maxx", c_float),
    ]


class Winding(IntEnum):
    CCW = 1  # Winding for solid shapes
    CW = 2  # Winding for holes
    SOLID = CCW
    HOLE = CW


@unique
class LineCap(IntEnum):
    BUTT = 0  # default
    ROUND = 1
    SQUARE = 2


@unique
class LineJoin(IntEnum):
    ROUND = 1
    BEVEL = 3
    MITER = 4  # default


@unique
class Align(IntFlag):
    ALIGN_LEFT = 1 << 0  # Default, align text horizontally to left.
    ALIGN_CENTER = 1 << 1  # Align text horizontally to center.
    ALIGN_RIGHT = 1 << 2  # Align text horizontally to right.
    ALIGN_TOP = 1 << 3  # Align text vertically to top.
    ALIGN_MIDDLE = 1 << 4  # Align text vertically to middle.
    ALIGN_BOTTOM = 1 << 5  # Align text vertically to bottom.
    ALIGN_BASELINE = 1 << 6  # Default, align text vertically to baseline.


@unique
class BlendFactor(IntFlag):
    ZERO = 1 << 0
    ONE = 1 << 1
    SRC_COLOR = 1 << 2
    ONE_MINUS_SRC_COLOR = 1 << 3
    DST_COLOR = 1 << 4
    ONE_MINUS_DST_COLOR = 1 << 5
    SRC_ALPHA = 1 << 6
    ONE_MINUS_SRC_ALPHA = 1 << 7
    DST_ALPHA = 1 << 8
    ONE_MINUS_DST_ALPHA = 1 << 9
    SRC_ALPHA_SATURATE = 1 << 10


@unique
class CompositeOperation(IntEnum):
    SOURCE_OVER = 0
    SOURCE_IN = 1
    SOURCE_OUT = 2
    ATOP = 3
    DESTINATION_OVER = 4
    DESTINATION_IN = 5
    DESTINATION_OUT = 6
    DESTINATION_ATOP = 7
    LIGHTER = 8
    COPY = 9
    XOR = 10


@unique
class ImageFlags(IntFlag):
    IMAGE_GENERATE_MIPMAPS = 1 << 0  # Generate mipmaps during creation of the image.
    IMAGE_REPEATX = 1 << 1  # Repeat image in X direction.
    IMAGE_REPEATY = 1 << 2  # Repeat image in Y direction.
    IMAGE_FLIPY = 1 << 3  # Flips (inverses) image in Y direction when rendered.
    IMAGE_PREMULTIPLIED = 1 << 4  # Image data has premultiplied alpha.
    IMAGE_NEAREST = 1 << 5  # Image interpolation is Nearest instead Linear


@unique
class CreationFlags(IntFlag):
    ANTIALIAS: int = 1 << 0
    """
    Flag indicating if geometry based anti-aliasing is used (may not be needed when using MSAA).
    """

    STENCIL_STROKES: int = 1 << 2
    """
    Flag indicating if strokes should be drawn using stencil buffer. The rendering will be a little
    slower, but path overlaps (i.e. self-intersecting or sharp turns) will be drawn just once.
    """

    DEBUG: int = 1 << 3
    """
    Flag indicating that additional debug checks are done.
    """


@_c_api_wrapper('nvgBeginFrame', [c_void_p, c_float, c_float, c_float])
def begin_frame(ctx: c_void_p, window_width: float, window_height: float, device_pixel_ratio: float):
    """
    Begin drawing a new frame
    Calls to nanovg drawing API should be wrapped in `begin_frame()` and`end_frame()`.
    `begin_frame()` defines the size of the window to render to in relation currently
    set viewport (i.e. glViewport on GL backends). Device pixel ration allows to
    control the rendering on Hi-DPI devices.
    For example, GLFW returns two dimension for an opened window: window size and
    frame buffer size. In that case you would set windowWidth/Height to the window size
    devicePixelRatio to: frameBufferWidth / windowWidth.

    :param ctx:                 nanovg context object.
    :param window_width:
    :param window_height:
    :param device_pixel_ratio:
    """
    _nanovg.nvgBeginFrame(ctx, window_width, window_height, device_pixel_ratio)


@_c_api_wrapper('nvgCancelFrame', [c_void_p])
def cancel_frame(ctx: c_void_p):
    """
    Cancels drawing the current frame.

    :param ctx:     nanovg context object.
    """
    _nanovg.nvgCancelFrame(ctx)


@_c_api_wrapper('nvgEndFrame', [c_void_p])
def end_frame(ctx: c_void_p):
    """
    Ends drawing flushing remaining render state.

    :param ctx:     nanovg context object.
    """
    _nanovg.nvgEndFrame(ctx)


@_c_api_wrapper('nvgGlobalCompositeOperation', [c_void_p, c_int32])
def global_composite_operation(ctx: c_void_p, op: CompositeOperation):
    """
    Sets the composite operation.
    The composite operations in NanoVG are modeled after HTML Canvas API, and
    the blend func is based on OpenGL (see corresponding manuals for more info).
    The colors in the blending state have premultiplied alpha.

    :param ctx:     nanovg context object.
    :param op:      Should be one of CompositeOperation.
    """
    _nanovg.nvgGlobalCompositeOperation(ctx, op)


@_c_api_wrapper('nvgGlobalCompositeBlendFunc', [c_void_p, c_int32, c_int32])
def global_composite_blend_func(ctx: c_void_p, src_factor: BlendFactor, dst_factor: BlendFactor):
    """
    Sets the composite operation with custom pixel arithmetic.

    :param ctx:         nanovg context object.
    :param src_factor:  Source factor.
    :param dst_factor:  Destination factor.
    """
    _nanovg.nvgGlobalCompositeBlendFunc(ctx, src_factor, dst_factor)


@_c_api_wrapper('nvgGlobalCompositeBlendFuncSeparate', [c_void_p, c_int32, c_int32, c_int32, c_int32])
def global_composite_blend_func_separate(ctx: c_void_p, src_rgb: BlendFactor, dst_rgb: BlendFactor,
                                         src_alpha: BlendFactor, dst_alpha: BlendFactor):
    """
    Sets the composite operation with custom pixel arithmetic for RGB and alpha components separately.

    :param ctx:         nanovg context object.
    :param src_rgb:     Source color factor.
    :param dst_rgb:     Destination color factor.
    :param src_alpha:   Source alpha factor.
    :param dst_alpha:   Destination alpha factor.
    """
    _nanovg.nvgGlobalCompositeBlendFuncSeparate(ctx, src_rgb, dst_rgb, src_alpha, dst_alpha)


@_c_api_wrapper('nvgRGBAf', [c_float, c_float, c_float, c_float], Color)
def rgba(r: float, g: float, b: float, a: float = 1) -> Color:
    """
    Returns a color value from red, green, blue and alpha values.

    :param r:   Red component in range [0...1].
    :param g:   Green component in range [0...1].
    :param b:   Blue component in range [0...1].
    :param a:   Alpha factor in range [0...1].
    :return:    New color.
    """
    return _nanovg.nvgRGBAf(r, g, b, a)


@_c_api_wrapper('nvgLerpRGBA', [Color, Color, c_float], Color)
def lerp_rgba(color0: Color, color1: Color, mix: float) -> Color:
    """
    Linearly interpolates from color c0 to c1, and returns resulting color value.

    :param color0:  Color at mix = 0.
    :param color1:  Color at mix = 1.
    :param mix:     Mix in range [0...1].
    :return:        Linearly interpolated color.
    """
    return _nanovg.nvgLerpRGBA(color0, color1, mix)


@_c_api_wrapper('nvgTransRGBAf', [Color, c_float], Color)
def trans_rgba(color: Color, alpha: float) -> Color:
    """
    Sets transparency of a color value.

    :param color:   Color to change
    :param alpha:   New alpha factor in range [0...1].
    :return:        Color with given alpha.
    """
    return _nanovg.nvgTransRGBAf(color, alpha)


@_c_api_wrapper('nvgHSLA', [c_float, c_float, c_float, c_ubyte], Color)
def hsla(hue: float, saturation: float, lightness: float, alpha: float = 1) -> Color:
    """
    Returns color value specified by hue, saturation and lightness and alpha.

    :param hue:         Hue in range [0...1].
    :param saturation:  Saturation in range [0...1].
    :param lightness:   Lightness in range [0...1].
    :param alpha:       Alpha in range [0...1].
    :return:            New color
    """
    return _nanovg.nvgHSLA(hue, saturation, lightness, round(max(0., min(1., alpha)) * 255.))


@_c_api_wrapper('nvgSave', [c_void_p])
def save(ctx: c_void_p):
    """
    Pushes and saves the current render state into a state stack.
    A matching restore() must be used to restore the state.

    NanoVG contains state which represents how paths will be rendered.
    The state contains transform, fill and stroke styles, text and font styles,
    and scissor clipping.

    :param ctx: nanovg context object.
    """
    _nanovg.nvgSave(ctx)


@_c_api_wrapper('nvgRestore', [c_void_p])
def restore(ctx: c_void_p):
    """
    Pops and restores current render state.

    :param ctx: nanovg context object.
    """
    _nanovg.nvgRestore(ctx)


@_c_api_wrapper('nvgReset', [c_void_p])
def reset(ctx: c_void_p):
    """
    Resets current render state to default values. Does not affect the render state stack.

    :param ctx: nanovg context object.
    """
    _nanovg.nvgReset(ctx)


@_c_api_wrapper('nvgShapeAntiAlias', [c_void_p, c_int32])
def shape_anti_alias(ctx: c_void_p, enabled: bool):
    """
    Sets whether to draw antialias for `stroke()` and `fill()`. It's enabled by default.

    :param ctx:     nanovg context object.
    :param enabled: True to draw antialiased strokes and fills, False otherwise.
    """
    _nanovg.nvgShapeAntiAlias(ctx, enabled)


@_c_api_wrapper('nvgStrokeColor', [c_void_p, Color])
def stroke_color(ctx: c_void_p, color: Color):
    """
    Sets current stroke style to a solid color.

    Fill and stroke render style can be either a solid color or a paint which is a gradient or a pattern.
    Solid color is simply defined as a color value, different kinds of paints can be created
    using `linear_gradient()`, `box_gradient()`, radial_gradient()` and `image_pattern()`.

    Current render style can be saved and restored using `save()` and `restore()`.

    :param ctx:     nanovg context object.
    :param color:   New stroke color.
    """
    _nanovg.nvgStrokeColor(ctx, color)


# Sets current stroke style to a paint, which can be a one of the gradients or a pattern.
@_c_api_wrapper('nvgStrokePaint', [c_void_p, Paint])
def stroke_paint(ctx: c_void_p, paint: Paint):
    _nanovg.nvgStrokePaint(ctx, paint)


@_c_api_wrapper('nvgFillColor', [c_void_p, Color])
def fill_color(ctx: c_void_p, color: Color):
    """
    Sets current fill style to a solid color.

    :param ctx:     nanovg context object.
    :param color:   Color to fill
    """
    _nanovg.nvgFillColor(ctx, color)


@_c_api_wrapper('nvgFillPaint', [c_void_p, Paint])
def fill_paint(ctx: c_void_p, paint: Paint):
    """
    Sets current fill style to a paint, which can be a one of the gradients or a pattern.

    :param ctx:     nanovg context object.
    :param paint:   Paint to fill
    """
    _nanovg.nvgFillPaint(ctx, paint)


@_c_api_wrapper('nvgMiterLimit', [c_void_p, c_float])
def miter_limit(ctx: c_void_p, limit: float):
    """
    Sets the miter limit of the stroke style.
    Miter limit controls when a sharp corner is beveled.

    :param ctx:
    :param limit:   Miter limit in untransformed pixels.
    """
    _nanovg.nvgMiterLimit(ctx, limit)


@_c_api_wrapper('nvgStrokeWidth', [c_void_p, c_float])
def stroke_width(ctx: c_void_p, width: float):
    """
    Sets the stroke width of the stroke style.

    :param ctx:
    :param width:   Stroke width in untransformed pixels
    """
    _nanovg.nvgStrokeWidth(ctx, width)


@_c_api_wrapper('nvgLineCap', [c_void_p, c_int32])
def line_cap(ctx: c_void_p, cap: LineCap):
    """
    Sets how the end of the line (cap) is drawn,

    :param ctx:
    :param cap: LineCap style.
    """
    _nanovg.nvgLineCap(ctx, cap)


@_c_api_wrapper('nvgLineJoin', [c_void_p, c_int32])
def line_join(ctx: c_void_p, join: LineJoin):
    """
    Sets how sharp path corners are drawn.

    :param ctx:
    :param join: LineJoin style.
    """
    _nanovg.nvgLineJoin(ctx, join)


@_c_api_wrapper('nvgGlobalAlpha', [c_void_p, c_float])
def global_alpha(ctx: c_void_p, alpha: float):
    """
    Sets the transparency applied to all rendered shapes.
    Already transparent paths will get proportionally more transparent as well.

    :param ctx:
    :param alpha:   New global alpha value.
    """
    _nanovg.nvgGlobalAlpha(ctx, alpha)


@_c_api_wrapper('nvgResetTransform', [c_void_p])
def reset_transform(ctx: c_void_p):
    """
    Resets current transform to a identity matrix.
    :param ctx:
    """
    _nanovg.nvgResetTransform(ctx)


@_c_api_wrapper('nvgTransform', [c_void_p, c_float, c_float, c_float, c_float, c_float, c_float])
def transform(ctx: c_void_p, a: float, b: float, c: float, d: float, e: float, f: float):
    """
    Premultiplies the current coordinate system by the matrix specified through the individual components.
    The parameters are interpreted as matrix as follows:

      [a c e]                          [sx kx tx]
      [b d f]   which corresponds to   [ky sy ty]
      [0 0 1]                          [ 0  0  1]

    Where: sx,sy define scaling, kx,ky skewing, and tx,ty translation.

    :param ctx:
    """
    _nanovg.nvgTransform(ctx, a, b, c, d, e, f)


@_c_api_wrapper('nvgTranslate', [c_void_p, c_float, c_float])
def translate(ctx: c_void_p, x: float, y: float):
    """
    Translates the current coordinate system.

    :param ctx:
    :param x:   Number of untransformed pixels to translate along the x axis.
    :param y:   Number of untransformed pixels to translate along the y axis.
    """
    _nanovg.nvgTranslate(ctx, x, y)


@_c_api_wrapper('nvgRotate', [c_void_p, c_float])
def rotate(ctx: c_void_p, angle: float):
    """
    Rotates the current coordinate system.

    :param ctx:
    :param angle:   Angle to rotate in radians.
    """
    _nanovg.nvgRotate(ctx, angle)


@_c_api_wrapper('nvgSkewX', [c_void_p, c_float])
def skew_x(ctx: c_void_p, angle: float):
    """
    Skews the current coordinate system along the x axis.

    :param ctx:
    :param angle:   Angle to skew in radians.
    """
    _nanovg.nvgSkewX(ctx, angle)


@_c_api_wrapper('nvgSkewY', [c_void_p, c_float])
def skew_y(ctx: c_void_p, angle: float):
    """
    Skews the current coordinate system along the y axis.

    :param ctx:
    :param angle:   Angle to skew in radians.
    """
    _nanovg.nvgSkewY(ctx, angle)


@_c_api_wrapper('nvgScale', [c_void_p, c_float, c_float])
def scale(ctx: c_void_p, x: float, y: float):
    """
    Scales the current coordinate system.

    :param ctx:
    :param x:   Horizontal scale factor.
    :param y:   Vertical scale factor.
    """
    _nanovg.nvgScale(ctx, x, y)


@_c_api_wrapper('nvgCurrentTransform', [c_void_p, c_void_p])
def current_transform(ctx: c_void_p) -> Xform:
    """
    Returns the part (a-f) of the current transformation matrix.
        [a c e]
        [b d f]
        [0 0 1]
    The last row is not returned as it is constant and implicit.

    :param ctx: nanovg context object.
    :return:    Xform containing a matrix of 6 floats representing (a, b, c, d, e, f).
    """
    xform = Xform()
    _nanovg.nvgCurrentTransform(ctx, xform)
    return xform


@_c_api_wrapper('nvgTransformIdentity', [c_void_p])
def transform_identity(xform: Xform):
    """
    Resets the given transform to the identity matrix in-place.
    :param xform:   Xform to reset.
    """
    _nanovg.nvgTransformIdentity(byref(xform))


@_c_api_wrapper('nvgTransformTranslate', [c_void_p, c_float, c_float])
def transform_translate(xform: Xform, tx: float, ty: float):
    """
    Sets the transform to a pure translation matrix in-place.

    :param xform:   Xform to modified.
    :param tx:      Translation along the x-axis.
    :param ty:      Translation along the y-axis.
    """
    _nanovg.nvgTransformTranslate(byref(xform), tx, ty)


@_c_api_wrapper('nvgTransformScale', [c_void_p, c_float, c_float])
def transform_scale(xform: Xform, sx: float, sy: float):
    """
    Sets the transform to a pure scale matrix in-place.

    :param xform:   Xform to modified.
    :param sx:      Horizontal scale factor.
    :param sy:      Vertical scale factor.
    """
    _nanovg.nvgTransformScale(xform, sx, sy)


@_c_api_wrapper('nvgTransformRotate', [c_void_p, c_float])
def transform_rotate(xform: Xform, angle: float):
    """
    Sets the transform to a pure rotate matrix in-place.

    :param xform:   Xform to modified.
    :param angle:   Rotation angle in radians.
    """
    _nanovg.nvgTransformRotate(byref(xform), angle)


@_c_api_wrapper('nvgTransformSkewX', [c_void_p, c_float])
def transform_skew_x(xform: Xform, angle: float):
    """
    Sets the transform to a pure skew matrix along the x-axis in-place.

    :param xform:   Xform to modified.
    :param angle:   Skew angle in radians.
    """
    _nanovg.nvgTransformSkewX(byref(xform), angle)


@_c_api_wrapper('nvgTransformSkewY', [c_void_p, c_float])
def transform_skew_y(xform: Xform, angle: float):
    """
    Sets the transform to a pure skew matrix along the y-axis in-place.

    :param xform:   Xform to modified.
    :param angle:   Skew angle in radians.
    """
    _nanovg.nvgTransformSkewY(byref(xform), angle)


@_c_api_wrapper('nvgTransformMultiply', [c_void_p, c_void_p])
def transform_multiply(dst: Xform, src: Xform):
    """
    Sets the transform to the result of multiplication of two transforms: `dst = dst*src`.

    :param dst: Matrix to modified.
    :param src: Matrix to multiply src with.
    """
    _nanovg.nvgTransformMultiply(byref(dst), byref(src))


@_c_api_wrapper('nvgTransformPremultiply', [c_void_p, c_void_p])
def transform_premultiply(dst: Xform, src: Xform):
    """
    Sets the transform to the result of multiplication of two transforms, of `dst = src*dst`.

    :param dst: Matrix to modified.
    :param src: Matrix to multiply src with.
    """
    _nanovg.nvgTransformPremultiply(byref(dst), byref(src))


@_c_api_wrapper('nvgTransformInverse', [c_void_p, c_void_p], c_int32)
def transform_inverse(xform: Xform) -> (Xform, bool):
    """
    Returns the inverse of the given transform

    :param xform:   Xform to invert
    :return:        First the inverse matrix and second a flag indicating whether the result is valid or not.
    """
    inverse = copy(xform)
    success = _nanovg.nvgTransformInverse(inverse, xform)
    return inverse, success == 1


@_c_api_wrapper('nvgTransformPoint', [c_void_p, c_void_p, c_void_p, c_float, c_float])
def transform_point(xform: Xform, x: float, y: float) -> (float, float):
    """
    Uses the given xform to transform a point.

    :param xform:   Transformation matrix.
    :param x:       X-position of the point.
    :param y:       Y-position of the point.
    :return:        Transformed position.
    """
    dest_x = c_float()
    dest_y = c_float()
    _nanovg.nvgTransformPoint(byref(dest_x), byref(dest_y), xform, x, y)
    return dest_x.value, dest_y.value


# Creates image by loading it from the disk from specified file name.
# Returns handle to the image.
@_c_api_wrapper('nvgCreateImage', [c_void_p, c_char_p, c_int32], c_int32)
def create_image(ctx: c_void_p, filename: str, image_flags: int) -> int:
    raise NotImplementedError()


# Creates image by loading it from the specified chunk of memory.
# Returns handle to the image.
@_c_api_wrapper('nvgCreateImageMem', [c_void_p, c_int32, c_void_p, c_int32], c_int32)
def create_image_mem(ctx: c_void_p, image_flags: int, data, ndata: int) -> int:
    raise NotImplementedError()


# Creates image from specified image data.
# Returns handle to the image.
@_c_api_wrapper('nvgCreateImageRGBA', [c_void_p, c_int32, c_int32, c_int32, c_void_p], c_int32)
def create_image_rgba(ctx: c_void_p, w: int, h: int, image_flags: int, data) -> int:
    raise NotImplementedError()


# Updates image data specified by image handle.
@_c_api_wrapper('nvgUpdateImage', [c_void_p, c_int32, c_void_p])
def update_image(ctx: c_void_p, image: int, data):
    raise NotImplementedError()


# Returns the dimensions of a created image.
@_c_api_wrapper('nvgImageSize', [c_void_p, c_int32, c_void_p, c_void_p])
def image_size(ctx: c_void_p, image) -> (int, int):
    raise NotImplementedError()


# Deletes created image.
@_c_api_wrapper('nvgDeleteImage', [c_void_p, c_int32])
def delete_image(ctx: c_void_p, image: int):
    raise NotImplementedError()


@_c_api_wrapper('nvgLinearGradient', [c_void_p, c_float, c_float, c_float, c_float, Color, Color], Paint)
def linear_gradient(ctx: c_void_p, start_x: float, start_y: float, end_x: float, end_y: float, start_color: Color,
                    end_color: Color) -> Paint:
    """
    Creates and returns a linear gradient.
    The gradient is transformed by the current transform when it is passed to `fill_paint()` or `stroke_paint()`.

    :param ctx:
    :param start_x:     X coordinate of the start point.
    :param start_y:     y coordinate of the start point.
    :param end_x:       X coordinate of the end point.
    :param end_y:       y coordinate of the end point.
    :param start_color: Start color of the gradient.
    :param end_color:   End color of the gradient
    :return:            New linear gradient paint.
    """
    return _nanovg.nvgLinearGradient(ctx, start_x, start_y, end_x, end_y, start_color, end_color)


@_c_api_wrapper('nvgBoxGradient', [c_void_p, c_float, c_float, c_float, c_float, c_float, c_float, Color, Color], Paint)
def box_gradient(
        ctx: c_void_p, left: float, top: float, width: float, height: float, radius: float, feather: float,
        inner_color: Color, outer_color: Color,
) -> Paint:
    """
    Creates and returns a box gradient.
    Box gradient is a feathered rounded rectangle, it is useful for rendering drop shadows or highlights for boxes.
    The gradient is transformed by the current transform when it is passed to `fill_paint()` or `stroke_paint()`.

    :param ctx:
    :param left:        Position of the left edge of the rectangle.
    :param top:         Position of the top edge of the rectangle.
    :param width:       Width of the rectangle.
    :param height:      Height of the rectangle.
    :param radius:      Radius of the round rectangle corners.
    :param feather:     Defines how blurry the border of the rectangle is.
    :param inner_color: Inner color of the gradient.
    :param outer_color: Outer color of the gradient.
    :return:            New box gradient paint.
    """
    return _nanovg.nvgBoxGradient(ctx, left, top, width, height, radius, feather, inner_color, outer_color)


@_c_api_wrapper('nvgRadialGradient', [c_void_p, c_float, c_float, c_float, c_float, Color, Color], Paint)
def radial_gradient(
        ctx: c_void_p, center_x: float, center_y: float, inner_radius: float, outer_radius: float, inner_color: Color,
        outer_color: Color
) -> Paint:
    """
    Creates and returns a radial gradient.
    The gradient is transformed by the current transform when it is passed to `fill_paint()` or `stroke_paint()`.

    :param ctx:
    :param center_x:        X position of the center.
    :param center_y:        Y position of the center.
    :param inner_radius:    Inner radius of the gradient.
    :param outer_radius:    Outer radius of the gradient.
    :param inner_color:     Inner color of the gradient.
    :param outer_color:     Outer color of the gradient.
    :return:
    """
    return _nanovg.nvgRadialGradient(ctx, center_x, center_y, inner_radius, outer_radius, inner_color, outer_color)


@_c_api_wrapper('nvgImagePattern', [c_void_p, c_float, c_float, c_float, c_float, c_float, c_int32, c_float], Paint)
def image_pattern(ctx: c_void_p, left: float, top: float, width: float, height: float, angle: float, image: int,
                  alpha: float,
                  ) -> Paint:
    """
    Creates and returns an image pattern.

    :param ctx:
    :param left:
    :param top:
    :param width:
    :param height:
    :param angle:
    :param image:
    :param alpha:
    :return:
    """
    raise NotImplementedError()


@_c_api_wrapper('nvgScissor', [c_void_p, c_float, c_float, c_float, c_float])
def scissor(ctx: c_void_p, left: float, top: float, width: float, height: float):
    """
    Sets the current scissor rectangle.
    Scissoring allows you to clip the rendering into a rectangle. This is useful for various user interface cases like
    rendering a text edit or a timeline.
    The scissor rectangle is transformed by the current transform.

    :param ctx:
    :param left:    X-position of the rectangles left edge.
    :param top:     Y-position of the rectangles top edge.
    :param width:   With of the rectangle.
    :param height:  Height of the rectangle.
    """
    _nanovg.nvgScissor(ctx, left, top, width, height)


@_c_api_wrapper('nvgIntersectScissor', [c_void_p, c_float, c_float, c_float, c_float])
def intersect_scissor(ctx: c_void_p, left: float, top: float, width: float, height: float):
    """
    Intersects the current scissor rectangle with the specified rectangle.
    The scissor rectangle is transformed by the current transform.
    In case the rotation of previous scissor rect differs from the current one, the intersection will be done between
    the specified rectangle and the previous scissor rectangle transformed in the current transform space.
    The resulting shape is always a rectangle.

    :param ctx:
    :param left:    X-position of the rectangles left edge.
    :param top:     Y-position of the rectangles top edge.
    :param width:   With of the rectangle.
    :param height:  Height of the rectangle.
    """
    _nanovg.nvgIntersectScissor(ctx, left, top, width, height)


@_c_api_wrapper('nvgResetScissor', [c_void_p])
def reset_scissor(ctx: c_void_p):
    """
    Reset and disables scissoring.

    :param ctx:
    """
    _nanovg.nvgResetScissor(ctx)


@_c_api_wrapper('nvgBeginPath', [c_void_p])
def begin_path(ctx: c_void_p):
    """
    Clears the current path and sub-paths.

    Drawing a new shape starts with `begin_path()`, it clears all the currently defined paths.
    Then you define one or more paths and sub-paths which describe the shape. The are functions
    to draw common shapes like rectangles and circles, and lower level step-by-step functions,
    which allow to define a path curve by curve.

    NanoVG uses even-odd fill rule to draw the shapes. Solid shapes should have counter clockwise
    winding and holes should have counter clockwise order. To specify winding of a path you can
    call `path_winding()`. This is useful especially for the common shapes, which are drawn CCW.

    Finally you can fill the path using current fill style by calling `fill()`, and stroke it
    with current stroke style by calling `stroke()`.

    The curve segments and sub-paths are transformed by the current transform.

    :param ctx: nanovg context object.
    """
    _nanovg.nvgBeginPath(ctx)


@_c_api_wrapper('nvgMoveTo', [c_void_p, c_float, c_float])
def move_to(ctx: c_void_p, x: float, y: float):
    """
    Starts a new sub-path with specified point as first point.

    :param ctx: nanovg context object.
    :param x:   X-position.
    :param y:   Y-position.
    """
    _nanovg.nvgMoveTo(ctx, x, y)


@_c_api_wrapper('nvgLineTo', [c_void_p, c_float, c_float])
def line_to(ctx: c_void_p, x: float, y: float):
    """
    Adds a line segment from the last point in the path to the specified point.

    :param ctx: nanovg context object.
    :param x:   X-position.
    :param y:   Y-position.
    """
    _nanovg.nvgLineTo(ctx, x, y)


@_c_api_wrapper('nvgBezierTo', [c_void_p, c_float, c_float, c_float, c_float, c_float, c_float])
def bezier_to(ctx: c_void_p, ctrl1_x: float, ctrl1_y: float, ctrl2_x: float, ctrl2_y: float, end_x: float,
              end_y: float):
    """
    Adds a cubic bezier segment from last point in the path via two control points to the specified point.

    :param ctx:     nanovg context object.
    :param ctrl1_x: X position of the first control point.
    :param ctrl1_y: Y position of the first control point.
    :param ctrl2_x: X position of the second control point.
    :param ctrl2_y: Y position of the second control point.
    :param end_x:   X position of the end point.
    :param end_y:   Y position of the end point.
    """
    _nanovg.nvgBezierTo(ctx, ctrl1_x, ctrl1_y, ctrl2_x, ctrl2_y, end_x, end_y)


@_c_api_wrapper('nvgQuadTo', [c_void_p, c_float, c_float, c_float, c_float])
def quad_to(ctx: c_void_p, ctrl_x: float, ctrl_y: float, end_x: float, end_y: float):
    """
    Adds a quadratic bezier segment from last point in the path via a control point to the specified point.

    :param ctx:     nanovg context object.
    :param ctrl_x:  X position of the control point.
    :param ctrl_y:  Y position of the control point.
    :param end_x:   X position of the end point.
    :param end_y:   Y position of the end point.
    """
    _nanovg.nvgQuadTo(ctx, ctrl_x, ctrl_y, end_x, end_y)


@_c_api_wrapper('nvgArcTo', [c_void_p, c_float, c_float, c_float, c_float, c_float])
def arc_to(ctx: c_void_p, x1: float, y1: float, x2: float, y2: float, radius: float):
    """
    Adds an arc segment at the corner defined by the last path point, and two specified points.

    :param ctx:     nanovg context object.
    :param x1:      X position of the first point.
    :param y1:      Y position of the first point.
    :param x2:      X position of the second point.
    :param y2:      Y position of the second point.
    :param radius:  Radius of the arc.
    """
    _nanovg.nvgArcTo(ctx, x1, y1, x2, y2, radius)


@_c_api_wrapper('nvgClosePath', [c_void_p])
def close_path(ctx: c_void_p):
    """
    Closes the current sub-path with a line segment.
    :param ctx: nanovg context object.
    """
    _nanovg.nvgClosePath(ctx)


@_c_api_wrapper('nvgPathWinding', [c_void_p, c_int32])
def path_winding(ctx: c_void_p, direction: Winding):
    """
    Sets the current sub-path winding, see Winding.

    :param ctx:         nanovg context object.
    :param direction:   Direction of the path winding.
    """
    _nanovg.nvgPathWinding(ctx, direction)


@_c_api_wrapper('nvgArc', [c_void_p, c_float, c_float, c_float, c_float, c_float, c_int32])
def arc(ctx: c_void_p, center_x: float, center_y: float, radius: float, sweep_start: float, sweep_end: float,
        direction: Winding):
    """
    Creates a new circle arc shaped sub-path.

    :param ctx:         nanovg context object.
    :param center_x:    X position of the arc center.
    :param center_y:    Y position of the arc center.
    :param radius:      Arc radius.
    :param sweep_start: Start angle of the sweep in radians.
    :param sweep_end:   End angle of the sweep in radians.
    :param direction:   Direction of the sweep.
    """
    _nanovg.nvgArc(ctx, center_x, center_y, radius, sweep_start, sweep_end, direction)


@_c_api_wrapper('nvgRect', [c_void_p, c_float, c_float, c_float, c_float])
def rect(ctx: c_void_p, left: float, top: float, width: float, height: float):
    """
    Creates a new rectangle shaped sub-path.

    :param ctx:     nanovg context object.
    :param left:    X position of the left edge of the rectangle.
    :param top:     Y position of the rop edge of the rectangle.
    :param width:   Width of the rectangle.
    :param height:  Height of the rectangle.
    """
    _nanovg.nvgRect(ctx, left, top, width, height)


@_c_api_wrapper('nvgRoundedRect', [c_void_p, c_float, c_float, c_float, c_float, c_float])
def rounded_rect(ctx: c_void_p, left: float, top: float, width: float, height: float, radius: float):
    """
    Creates a new rounded rectangle shaped sub-path.

    :param ctx:     nanovg context object.
    :param left:    X position of the left edge of the rectangle.
    :param top:     Y position of the rop edge of the rectangle.
    :param width:   Width of the rectangle.
    :param height:  Height of the rectangle.
    :param radius:  Radius of the corners in pixels.
    """
    _nanovg.nvgRoundedRect(ctx, left, top, width, height, radius)


@_c_api_wrapper('nvgRoundedRectVarying',
                [c_void_p, c_float, c_float, c_float, c_float, c_float, c_float, c_float, c_float])
def rounded_rect_varying(ctx: c_void_p, left: float, top: float, width: float, height: float, radius_tl: float,
                         radius_tr: float, radius_br: float, radius_bl: float):
    """
    Creates a new rounded rectangle shaped sub-path with varying radii for each corner.

    :param ctx:         nanovg context object.
    :param left:        X position of the left edge of the rectangle.
    :param top:         Y position of the rop edge of the rectangle.
    :param width:       Width of the rectangle.
    :param height:      Height of the rectangle.
    :param radius_tl:   Radius of the top-left corner.
    :param radius_tr:   Radius of the top-right corner.
    :param radius_br:   Radius of the bottom-right corner.
    :param radius_bl:   Radius of the bottom-left corner.
    """
    _nanovg.nvgRoundedRectVarying(ctx, left, top, width, height, radius_tl, radius_tr, radius_br, radius_bl)


@_c_api_wrapper('nvgEllipse', [c_void_p, c_float, c_float, c_float, c_float])
def ellipse(ctx: c_void_p, center_x: float, center_y: float, radius_x: float, radius_y: float):
    """
    Creates a new ellipse shaped sub-path.

    :param ctx:         nanovg context object.
    :param center_x:    X position of the center of the ellipse.
    :param center_y:    Y position of the center of the ellipse.
    :param radius_x:    Horizontal radius of the ellipse.
    :param radius_y:    Vertical radius of the ellipse.
    """
    _nanovg.nvgEllipse(ctx, center_x, center_y, radius_x, radius_y)


@_c_api_wrapper('nvgCircle', [c_void_p, c_float, c_float, c_float])
def circle(ctx: c_void_p, center_x: float, center_y: float, radius):
    """
    Creates a new circle shaped sub-path.

    :param ctx:         nanovg context object.
    :param center_x:    X position of the center of the circle.
    :param center_y:    Y position of the center of the circle.
    :param radius:      Radius of the circle.
    """
    _nanovg.nvgCircle(ctx, center_x, center_y, radius)


@_c_api_wrapper('nvgFill', [c_void_p])
def fill(ctx: c_void_p):
    """
    Fills the current path with current fill style.

    :param ctx: nanovg context object.
    """
    _nanovg.nvgFill(ctx)


@_c_api_wrapper('nvgStroke', [c_void_p])
def stroke(ctx: c_void_p):
    """
    Fills the current path with current stroke style.

    :param ctx: nanovg context object.
    """
    _nanovg.nvgStroke(ctx)


# Creates font by loading it from the disk from specified file name.
# Returns handle to the font.
@_c_api_wrapper('nvgCreateFont', [c_void_p, c_char_p, c_char_p], c_int32)
def create_font(ctx: c_void_p, name: str, filename: str) -> int:
    raise NotImplementedError()


# Creates font by loading it from the specified memory chunk.
# Returns handle to the font.
@_c_api_wrapper('nvgCreateFontMem', [c_void_p, c_char_p, c_void_p, c_int32, c_int32], c_int32)
def create_font_mem(ctx: c_void_p, name: str, data, ndata: int, free_data: int) -> int:
    raise NotImplementedError()


# Finds a loaded font of specified name, and returns handle to it, or -1 if the font is not found.
@_c_api_wrapper('nvgFindFont', [c_void_p, c_char_p], c_int32)
def find_font(ctx: c_void_p, name: str) -> int:
    raise NotImplementedError()


# Adds a fallback font by handle.
@_c_api_wrapper('nvgAddFallbackFontId', [c_void_p, c_int32, c_int32], c_int32)
def add_fallback_font_id(ctx: c_void_p, base_font: int, fallback_font: int) -> int:
    raise NotImplementedError()


# Adds a fallback font by name.
@_c_api_wrapper('nvgAddFallbackFont', [c_void_p, c_char_p, c_char_p], c_int32)
def add_fallback_font(ctx: c_void_p, base_font: str, fallback_font: str):
    raise NotImplementedError()


# Sets the font size of current text style.
@_c_api_wrapper('nvgFontSize', [c_void_p, c_float])
def font_size(ctx: c_void_p, size: float):
    raise NotImplementedError()


# Sets the blur of current text style.
@_c_api_wrapper('nvgFontBlur', [c_void_p, c_float])
def font_blur(ctx: c_void_p, blur: float):
    raise NotImplementedError()


# Sets the letter spacing of current text style.
@_c_api_wrapper('nvgTextLetterSpacing', [c_void_p, c_float])
def text_letter_spacing(ctx: c_void_p, spacing: float):
    raise NotImplementedError()


# Sets the proportional line height of current text style. The line height is specified as multiple of font size.
@_c_api_wrapper('nvgTextLineHeight', [c_void_p, c_float])
def text_line_height(ctx: c_void_p, line_height: float):
    raise NotImplementedError()


# Sets the text align of current text style, see NVGalign for options.
@_c_api_wrapper('nvgTextAlign', [c_void_p, c_int32])
def text_align(ctx: c_void_p, align: int):
    raise NotImplementedError()


# Sets the font face based on specified id of current text style.
@_c_api_wrapper('nvgFontFaceId', [c_void_p, c_int32])
def font_face_id(ctx: c_void_p, font: int):
    raise NotImplementedError()


# Sets the font face based on specified name of current text style.
@_c_api_wrapper('nvgFontFace', [c_void_p, c_char_p])
def font_face(ctx: c_void_p, font: str):
    raise NotImplementedError()


# Draws text string at specified location. If end is specified only the sub-string up to the end is drawn.
@_c_api_wrapper('nvgText', [c_void_p, c_float, c_float, c_char_p, c_char_p], c_float)
def text(ctx: c_void_p, x: float, y: float, string: str, end: str) -> float:
    raise NotImplementedError()


# Draws multi-line text string at specified location wrapped at the specified width. If end is specified only the
# sub-string up to the end is drawn. White space is stripped at the beginning of the rows, the text is split at word
# boundaries or when new-line characters are encountered.
# Words longer than the max width are slit at nearest character (i.e. no hyphenation).
@_c_api_wrapper('nvgTextBox', [c_void_p, c_float, c_float, c_float, c_char_p, c_char_p])
def text_box(ctx: c_void_p, x: float, y: float, break_row_width: float, string: str, end: str):
    raise NotImplementedError()


# Measures the specified text string. Parameter bounds should be a pointer to float[4],
# if the bounding box of the text should be returned. The bounds value are [xmin,ymin, xmax,ymax]
# Returns the horizontal advance of the measured text (i.e. where the next character should drawn).
# Measured values are returned in local coordinate space.
@_c_api_wrapper('nvgTextBounds', [c_void_p, c_float, c_float, c_char_p, c_char_p, c_void_p], c_float)
def text_bounds(ctx: c_void_p, x: float, y: float, string: str, end: str, bounds) -> float:
    raise NotImplementedError()


# Measures the specified multi-text string. Parameter bounds should be a pointer to float[4],
# if the bounding box of the text should be returned. The bounds value are [xmin,ymin, xmax,ymax]
# Measured values are returned in local coordinate space.
@_c_api_wrapper('nvgTextBoxBounds', [c_void_p, c_float, c_float, c_float, c_char_p, c_char_p, c_void_p])
def text_box_bounds(ctx: c_void_p, x: float, y: float, break_row_width: float, string: str, end: str, bounds):
    raise NotImplementedError()


# Calculates the glyph x positions of the specified text. If end is specified only the sub-string will be used.
# Measured values are returned in local coordinate space.
@_c_api_wrapper('nvgTextGlyphPositions', [c_void_p, c_float, c_float, c_char_p, c_char_p, c_void_p, c_int32], c_int32)
def text_glyph_positions(ctx: c_void_p, x: float, y: float, string: str, end: str, positions: GlyphPosition,
                         max_positions: int) -> int:
    raise NotImplementedError()


# Returns the vertical metrics based on the current text style.
# Measured values are returned in local coordinate space.
# Returns ascender, descender, line height
@_c_api_wrapper('nvgTextMetrics', [c_void_p, c_void_p, c_void_p, c_void_p])
def text_metrics(ctx: c_void_p) -> (float, float, float):
    raise NotImplementedError()


# Breaks the specified text into lines. If end is specified only the sub-string will be used.
# White space is stripped at the beginning of the rows, the text is split at word boundaries or when new-line characters
# are encountered.
# Words longer than the max width are slit at nearest character (i.e. no hyphenation).
@_c_api_wrapper('nvgTextBreakLines', [c_void_p, c_char_p, c_char_p, c_float, c_void_p, c_int32], c_int32)
def text_break_lines(ctx: c_void_p, string: str, end: str, break_row_width: float, rows: TextRow, max_rows: int):
    raise NotImplementedError()
