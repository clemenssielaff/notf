#pragma once

#include <nanovg/nanovg.h>

#include "common/aabr.hpp"
#include "common/color.hpp"
#include "common/transform2.hpp"
#include "common/vector2.hpp"
#include "graphics/rendercontext.hpp"
#include "utils/enum_to_number.hpp"

namespace notf {

class Widget;

/** A Painter is an object that paints into a Widget's CanvasComponent.
 *
 * This is a thin wrapper around C methods exposed from nanovg.h written by Mikko Mononen.
 * Some of the docstrings are verbatim copies of their corresponding nvg documentation.
 */
class Painter final {

    friend class CanvasComponent; // may construct a Painter

public: // enums
    enum class Winding {
        CCW = NVG_CCW,
        CW = NVG_CW,
    };

    enum class LineCap : int {
        BUTT = NVG_BUTT, // default
        ROUND = NVG_ROUND,
        SQUARE = NVG_SQUARE,
    };

    enum class LineJoin : int {
        ROUND = NVG_ROUND,
        BEVEL = NVG_BEVEL,
        MITER = NVG_MITER, // default
    };

    enum class Align {
        LEFT = NVG_ALIGN_LEFT, // default
        CENTER = NVG_ALIGN_CENTER,
        RIGHT = NVG_ALIGN_RIGHT,

        TOP = NVG_ALIGN_TOP,
        MIDDLE = NVG_ALIGN_MIDDLE,
        BOTTOM = NVG_ALIGN_BOTTOM,
        BASELINE = NVG_ALIGN_BASELINE, // default
    };

    /** Input for set_composite()
     * Modelled after the HTML Canvas API as described in https://www.w3.org/TR/2dcontext/#compositing
     */
    enum class Composite : int {
        SOURCE_OVER = NVG_SOURCE_OVER,
        SOURCE_IN = NVG_SOURCE_IN,
        SOURCE_OUT = NVG_SOURCE_OUT,
        ATOP = NVG_ATOP,
        DESTINATION_OVER = NVG_DESTINATION_OVER,
        DESTINATION_IN = NVG_DESTINATION_IN,
        DESTINATION_OUT = NVG_DESTINATION_OUT,
        DESTINATION_ATOP = NVG_DESTINATION_ATOP,
        LIGHTER = NVG_LIGHTER,
        COPY = NVG_COPY,
        XOR = NVG_XOR,
    };

    enum class ImageFlags {
        GENERATE_MIPMAPS = NVG_IMAGE_GENERATE_MIPMAPS,
        REPEATX = NVG_IMAGE_REPEATX,
        REPEATY = NVG_IMAGE_REPEATY,
        FLIPY = NVG_IMAGE_FLIPY,
        PREMULTIPLIED = NVG_IMAGE_PREMULTIPLIED,
    };

protected: // methods for CanvasComponent
    explicit Painter(const Widget* widget, const RenderContext* context)
        : m_widget(widget)
        , m_context(context)
    {
    }

public: // methods
    ~Painter()
    {
        nvgClear(_get_context()); // TODO: check if this actually works (and if not calling it would affect subsequent renderers)
    }

    /* Context ********************************************************************************************************/

    // TODO: inspection methods for the widget and render context

    /* State Handling *************************************************************************************************/

    /** Saves the current render state onto a stack.
     * A matching state_restore() must be used to restore the state.
     */
    void save_state() { nvgSave(_get_context()); }

    /** Pops and restores current render state. */
    void restore_state() { nvgRestore(_get_context()); }

    /** Resets current render state to default values. Does not affect the render state stack. */
    void reset_state() { nvgReset(_get_context()); }

    /* Composite ******************************************************************************************************/

    /** Determines how incoming (source) pixels are combined with existing (destination) pixels. */
    void set_composite(Composite composite) { nvgGlobalCompositeOperation(_get_context(), to_number(composite)); }

    /** Sets the global transparency of all rendered shapes. */
    void set_alpha(float alpha) { nvgGlobalAlpha(_get_context(), alpha); }

    /* Style **********************************************************************************************************/

    /** Sets the current stroke style to a solid color. */
    void set_stroke(const Color& color) { nvgStrokeColor(_get_context(), _to_nvg(color)); }

    /** Sets the current stroke style to a paint. */
    void set_stroke(NVGpaint paint) { nvgStrokePaint(_get_context(), std::move(paint)); } // TODO: move set_* into fill / stroke (meaning, call fill(paint) instead of set_paint(paint); fill();)

    /** Sets the current fill style to a solid color. */
    void set_fill(const Color& color) { nvgFillColor(_get_context(), _to_nvg(color)); }

    /** Sets the current fill style to a paint. */
    void set_fill(NVGpaint paint) { nvgFillPaint(_get_context(),  std::move(paint)); }

    /** Sets the width of the stroke. */
    void set_stroke_width(float width) { nvgStrokeWidth(_get_context(), width); }

    /** Sets how the end of the line (cap) is drawn - default is LineCap::BUTT. */
    void set_line_cap(LineCap cap) { nvgLineCap(_get_context(), to_number(cap)); }

    /** Sets how sharp path corners are drawn - default is LineJoin::MITER. */
    void set_line_join(LineJoin join) { nvgLineJoin(_get_context(), to_number(join)); }

    /** Sets the miter limit of the stroke. */
    void set_miter_limit(float limit) { nvgMiterLimit(_get_context(), limit); }

    /* Paints *********************************************************************************************************/

    /** Creates a linear gradient paint.
     * Coordinates are applied in the transformed coordinate system current at painting.
     */
    NVGpaint LinearGradient(float sx, float sy, float ex, float ey, const Color& start_color, const Color& end_color)
    {
        return nvgLinearGradient(_get_context(), sx, sy, ex, ey, _to_nvg(start_color), _to_nvg(end_color));
    }
    NVGpaint LinearGradient(const Vector2& start_pos, const Vector2& end_pos, const Color& start_color, const Color& end_color)
    {
        return LinearGradient(start_pos.x, start_pos.y, end_pos.x, end_pos.y, start_color, end_color);
    }

    /** Creates a box gradient paint - a feathered, rounded rectangle; useful for example as drop shadows for boxes.
     * Coordinates are applied in the transformed coordinate system current at painting.
     * @param x             Top-left x of the box.
     * @param y             Top-left y of the box.
     * @param w             Width of the box.
     * @param h             Height of the box.
     * @param radius        Corner radius.
     * @param feather       How blurry the border of the rectangle is drawn.
     * @param inner_color   Color on the inside of the box.
     * @param outer_color   Color on the outside of the box.
     */
    NVGpaint BoxGradient(float x, float y, float w, float h, float radius, float feather,
                         const Color& inner_color, const Color& outer_color)
    {
        return nvgBoxGradient(_get_context(), x, y, w, h, radius, feather, _to_nvg(inner_color), _to_nvg(outer_color));
    }
    NVGpaint BoxGradient(const Aabr& box, float radius, float feather, const Color& inner_color, const Color& outer_color)
    {
        return nvgBoxGradient(_get_context(), box.left(), box.top(), box.width(), box.height(), radius, feather,
                              _to_nvg(inner_color), _to_nvg(outer_color));
    }

    // TODO: circle class

    /** Creates a radial gradient paint. */
    NVGpaint RadialGradient(float cx, float cy, float inr, float outr, const Color& inner_color, const Color& outer_color)
    {
        return nvgRadialGradient(_get_context(), cx, cy, inr, outr, _to_nvg(inner_color), _to_nvg(outer_color));
    }

    /* Transform ******************************************************************************************************/

    /** Resets the coordinate system to its identity. */
    void reset_transform() { nvgResetTransform(_get_context()); }

    /** Translates the coordinate system. */
    void translate(float x, float y) { nvgTranslate(_get_context(), x, y); }
    void translate(const Vector2& vec) { nvgTranslate(_get_context(), vec.x, vec.y); }

    /** Rotates the coordinate system `angle` radians in a clockwise direction. */
    void rotate(float angle) { nvgRotate(_get_context(), angle); }

    /** Scales the coordinate system uniformly in both x- and y. */
    void scale(float factor) { nvgScale(_get_context(), factor, factor); }

    /** Scales the coordinate system in x- and y independently. */
    void scale(float x, float y) { nvgScale(_get_context(), x, y); }

    /** Skews the coordinate system along x for `angle` radians. */
    void skew_x(float angle) { nvgSkewX(_get_context(), angle); }

    /** Skews the coordinate system along y for `angle` radians. */
    void skew_y(float angle) { nvgSkewY(_get_context(), angle); }

    /** The transformation matrix of the coordinate system. */
    Transform2 get_transform()
    {
        Transform2 result;
        nvgCurrentTransform(_get_context(), result.as_ptr());
        return result;
    }

    /** Sets the transformation matrix of the coordinate system. */
    void set_transform(const Transform2& transform)
    {
        nvgTransform(_get_context(),
                     transform[0][0], transform[0][1], transform[0][1],
                     transform[1][0], transform[1][1], transform[1][2]);
    }

    /* Images *********************************************************************************************************/

    // TODO: painter image handling (in combination with the resource manager)

    /* Scissoring *****************************************************************************************************/

    /** Limits all painting to the inside of the given (transformed) rectangle. */
    void set_scissor(float x, float y, float width, float height) { nvgScissor(_get_context(), x, y, width, height); }
    void set_scissor(const Aabr& aabr) { nvgScissor(_get_context(), aabr.left(), aabr.top(), aabr.width(), aabr.height()); }

    /** Intersects the current scissor with the given rectangle, both in the same (transformed) coordinate system. */
    void intersect_scissor(float x, float y, float width, float height) { nvgIntersectScissor(_get_context(), x, y, width, height); }
    void intersect_scissor(const Aabr& aabr) { nvgIntersectScissor(_get_context(), aabr.left(), aabr.top(), aabr.width(), aabr.height()); }

    /** Resets the scissor rectangle and disables scissoring. */
    void reset_scissor() { nvgResetScissor(_get_context()); }

    /* Paths **********************************************************************************************************/

    /** Clears the current path and sub-paths. */
    void start_path() { nvgBeginPath(_get_context()); }

    /** Sets the current sub-path winding. */
    void set_winding(Winding winding) { nvgPathWinding(_get_context(), to_number(winding)); }

    /** Starts new sub-path with specified point as first point. */
    void move_to(float x, float y) { nvgMoveTo(_get_context(), x, y); }
    void move_to(const Vector2& pos) { nvgMoveTo(_get_context(), pos.x, pos.y); }

    /** Adds line segment from the last point in the path to the specified point. */
    void line_to(float x, float y) { nvgLineTo(_get_context(), x, y); }
    void line_to(const Vector2& pos) { nvgLineTo(_get_context(), pos.x, pos.y); }

    /** Adds cubic bezier segment from last point in the path via two control points to the specified point. */
    void bezier_to(float c1x, float c1y, float c2x, float c2y, float x, float y) { nvgBezierTo(_get_context(), c1x, c1y, c2x, c2y, x, y); }
    void bezier_to(const Vector2& ctrl_1, const Vector2& ctrl_2, const Vector2& end) { nvgBezierTo(_get_context(), ctrl_1.x, ctrl_1.y, ctrl_2.x, ctrl_2.y, end.x, end.y); }

    /** Adds quadratic bezier segment from last point in the path via a control point to the specified point. */
    void quad_to(float cx, float cy, float x, float y) { nvgQuadTo(_get_context(), cx, cy, x, y); }
    void quad_to(const Vector2& ctrl, const Vector2& end) { nvgQuadTo(_get_context(), ctrl.x, ctrl.y, end.x, end.y); }

    /** Adds an arc segment at the corner defined by the last path point, and two specified points. */
    void arc_to(float x1, float y1, float x2, float y2, float radius) { nvgArcTo(_get_context(), x1, y1, x2, y2, radius); }
    void arc_to(const Vector2& ctrl, const Vector2& end, float radius) { nvgArcTo(_get_context(), ctrl.x, ctrl.y, end.x, end.y, radius); }

    /** Creates new circle arc shaped sub-path.
     *
     * The arc center is at cx,cy, the arc radius is r, the arc is drawn from angle a 0 to a1,
     * and swept either clockwise or counter-clockwise.
     * Angles are specified in radians.
     */
    void arc(float cx, float cy, float r, float a0, float a1, Winding winding) { nvgArc(_get_context(), cx, cy, r, a0, a1, to_number(winding)); }
    void arc(const Vector2& pos, float radius, float start_angle, float end_angle, Winding direction)
    {
        nvgArc(_get_context(), pos.x, pos.y, radius, start_angle, end_angle, to_number(direction));
    }

    /** Creates new rectangle shaped sub-path. */
    void rect(float x, float y, float w, float h) { nvgRect(_get_context(), x, y, w, h); }
    void rect(const Aabr& aabr) { nvgRect(_get_context(), aabr.left(), aabr.top(), aabr.width(), aabr.height()); }

    /** Creates new rounded rectangle shaped sub-path. */
    void rounded_rect(float x, float y, float w, float h, float r) { nvgRoundedRect(_get_context(), x, y, w, h, r); }
    void rounded_rect(float x, float y, float w, float h, float rad_nw, float rad_ne, float rad_se, float rad_sw)
    {
        nvgRoundedRectVarying(_get_context(), x, y, w, h, rad_nw, rad_ne, rad_se, rad_sw);
    }
    void rounded_rect(const Aabr& aabr, float radius) { nvgRoundedRect(_get_context(), aabr.left(), aabr.top(), aabr.width(), aabr.height(), radius); }
    void rounded_rect(const Aabr& aabr, float radTopLeft, float radTopRight, float radBottomRight, float radBottomLeft)
    {
        nvgRoundedRectVarying(_get_context(), aabr.left(), aabr.top(), aabr.width(), aabr.height(), radTopLeft, radTopRight, radBottomRight, radBottomLeft);
    }

    /** Creates new ellipse shaped sub-path. */
    void ellipse(float cx, float cy, float rx, float ry) { nvgEllipse(_get_context(), cx, cy, rx, ry); }
    void ellipse(const Aabr& aabr) { nvgEllipse(_get_context(), aabr.x(), aabr.y(), aabr.width(), aabr.height()); }

    /** Creates new circle shaped sub-path. */
    void circle(float cx, float cy, float r) { nvgCircle(_get_context(), cx, cy, r); }
    void circle(const Vector2& pos, float radius) { nvgCircle(_get_context(), pos.x, pos.y, radius); }

    /** Closes current sub-path with a line segment. */
    void close_path() { nvgClosePath(_get_context()); }

    /** Fills the current path with current fill style. */
    void fill() { nvgFill(_get_context()); }

    /** Fills the current path with current stroke style. */
    void stroke() { nvgStroke(_get_context()); }

    /* Text ***********************************************************************************************************/

    // TODO: text rendering

private: // methods
    /** Convenienct access to the NanoVG context. */
    NVGcontext* _get_context() { return m_context->nanovg_context; }

    /** notf::Color -> NVGcolor. */
    static NVGcolor _to_nvg(const Color& color) { return {{{color.r, color.g, color.b, color.a}}}; }

private: // fields
    /** The Widget rendered into. */
    const Widget* m_widget;

    /** The RenderContext used for painting. */
    const RenderContext* m_context;
};

} // namespace notf
