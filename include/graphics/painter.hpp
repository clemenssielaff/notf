#pragma once

#include <nanovg/nanovg.h>

#include "common/aabr.hpp"
#include "common/circle.hpp"
#include "common/color.hpp"
#include "common/size2f.hpp"
#include "common/transform2.hpp"
#include "common/vector2.hpp"
#include "graphics/font.hpp"
#include "graphics/rendercontext.hpp"
#include "graphics/texture2.hpp"
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

    enum Align {
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
    enum Composite : int {
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

protected: // methods for CanvasComponent
    /**
     * @param widget    Drawn widget.
     * @param context   Render context.
     */
    Painter(const Widget* widget, const RenderContext* context);

public: // methods
    ~Painter();

    /* Context ********************************************************************************************************/

    /** Returns the size of the Widget in local coordinates. */
    Size2f get_widget_size() const;

    /** Returns the size of the Window's framebuffer in pixels. */
    Size2i get_buffer_size() const { return m_context->buffer_size; }

    /** Returns the mouse position in the Widget's coordinate system. */
    Vector2 get_mouse_pos() const;

    /** Returns the time since Application start in seconds. */
    double get_time() const { return m_context->time.in_seconds(); }

    /* State Handling *************************************************************************************************/

    /** Saves the current render state onto a stack.
     * A matching state_restore() must be used to restore the state.
     */
    void save_state()
    {
        nvgSave(_get_context());
        ++m_state_count;
    }

    /** Pops and restores current render state. */
    void restore_state()
    {
        if (m_state_count > 2) { // do not pop original or base-state (see constructor for details)
            nvgRestore(_get_context());
        }
    }

    /** Resets current render state to default values. Does not affect the render state stack. */
    void reset_state() { nvgReset(_get_context()); }

    /* Composite ******************************************************************************************************/

    /** Determines how incoming (source) pixels are combined with existing (destination) pixels. */
    void set_composite(Composite composite) { nvgGlobalCompositeOperation(_get_context(), composite); }

    /** Sets the global transparency of all rendered shapes. */
    void set_alpha(float alpha) { nvgGlobalAlpha(_get_context(), alpha); }

    /* Style **********************************************************************************************************/

    /** Sets the current stroke style to a solid color. */
    void set_stroke(const Color& color) { nvgStrokeColor(_get_context(), _to_nvg(color)); }

    /** Sets the current stroke style to a paint. */
    void set_stroke(NVGpaint paint) { nvgStrokePaint(_get_context(), std::move(paint)); }

    /** Sets the current fill style to a solid color. */
    void set_fill(const Color& color) { nvgFillColor(_get_context(), _to_nvg(color)); }

    /** Sets the current fill style to a paint. */
    void set_fill(NVGpaint paint) { nvgFillPaint(_get_context(), std::move(paint)); }

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

    /** Creates a radial gradient paint. */
    NVGpaint RadialGradient(float cx, float cy, float inr, float outr, const Color& inner_color, const Color& outer_color)
    {
        return nvgRadialGradient(_get_context(), cx, cy, inr, outr, _to_nvg(inner_color), _to_nvg(outer_color));
    }
    NVGpaint RadialGradient(const Vector2& center, float inr, float outr, const Color& inner_color, const Color& outer_color)
    {
        return nvgRadialGradient(_get_context(), center.x, center.y, inr, outr, _to_nvg(inner_color), _to_nvg(outer_color));
    }

    /** Creates an image paint. */
    NVGpaint ImagePattern(const std::shared_ptr<Texture2>& texture, float offset_x = 0, float offset_y = 0, float width = -1, float height = -1, float angle = 0)
    {
        if (width < 0 || height < 0) {
            const Size2i pixel_size = texture->get_size();
            width = static_cast<float>(pixel_size.width);
            height = static_cast<float>(pixel_size.height);
        }
        return nvgImagePattern(_get_context(), offset_x, offset_y, width, height, angle, texture->get_id(), 1.f); // alpha = 1
    }
    NVGpaint ImagePattern(const std::shared_ptr<Texture2>& texture, const Vector2& offset = {0, 0}, Size2f size = {NAN, NAN}, float angle = 0.f)
    {
        if (!size.is_valid()) {
            size = Size2f::from_size2i(texture->get_size());
        }
        return nvgImagePattern(_get_context(), offset.x, offset.y, size.width, size.height, angle, texture->get_id(), 1.f); // alpha = 1
    }
    NVGpaint ImagePattern(const std::shared_ptr<Texture2>& texture, const Aabr& aabr, float angle = 0.f)
    {
        return nvgImagePattern(_get_context(), aabr.left(), aabr.top(), aabr.width(), aabr.height(), angle, texture->get_id(), 1.f); // alpha = 1
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
    Transform2 get_transform() // TODO: python bindings for Transform2
    {
        Transform2 result;
        nvgCurrentTransform(_get_context(), result.as_ptr());
        return result;
    }

    /** Sets the transformation matrix of the coordinate system. */
    void set_transform(const Transform2& transform)
    {
        nvgTransform(_get_context(),
                     transform[0][0], transform[1][0], transform[0][1],
                     transform[1][1], transform[0][2], transform[1][2]);
    }

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

    /** Clears the current path and sub-paths and begins a new one. */
    void begin() { nvgBeginPath(_get_context()); }

    /** Sets the current sub-path winding. */
    void set_winding(Winding winding) { nvgPathWinding(_get_context(), to_number(winding)); }

    /** Starts new sub-path with specified point as first point. */
    void move_to(float x, float y) { nvgMoveTo(_get_context(), x, y); }
    void move_to(const Vector2& pos) { nvgMoveTo(_get_context(), pos.x, pos.y); }

    /** Closes current sub-path with a line segment. */
    void close() { nvgClosePath(_get_context()); }

    /** Fills the current path with current fill style. */
    void fill() { nvgFill(_get_context()); }

    /** Fills the current path with current stroke style. */
    void stroke() { nvgStroke(_get_context()); }

    /* Shapes *********************************************************************************************************/

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
    void arc(const Circle& circle, float start_angle, float end_angle, Winding direction)
    {
        nvgArc(_get_context(), circle.center.x, circle.center.y, circle.radius, start_angle, end_angle, to_number(direction));
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
    void circle(const Circle& circle) { nvgCircle(_get_context(), circle.center.x, circle.center.y, circle.radius); }

    /* Text ***********************************************************************************************************/

    /** Sets the font size of the current text style. */
    void set_font_size(float size) { nvgFontSize(_get_context(), size); }

    /** Sets the blur of the current text style. */
    void set_font_blur(float blur) { nvgFontBlur(_get_context(), blur); }

    /** Sets the letter spacing of the current text style. */
    void set_letter_spacing(float spacing) { nvgTextLetterSpacing(_get_context(), spacing); }

    /** Sets the proportional line height of the current text style.
     * The line height is specified as multiple of font size.
     */
    void set_line_height(float height) { nvgTextLineHeight(_get_context(), height); }

    /** Sets the text align of the current text style. */
    void set_text_align(Align align) { nvgTextAlign(_get_context(), align); }

    /** Sets the font of the current text style. */
    void set_font(const std::shared_ptr<Font>& font) { nvgFontFaceId(_get_context(), font->get_id()); }

    /** Draws a text at the specified location up to `length` characters long.
     * @param x         X-coordinate.
     * @param y         Y-coordinate.
     * @param string    Text to draw.
     * @param length    Number of characters to print (defaults to 0 = all).
     * @return          The x-coordinate of a hypothetical next character in local space.
     */
    float text(float x, float y, const std::string& string, size_t length = 0)
    {
        return nvgText(_get_context(), x, y, string.c_str(), (length == 0) ? nullptr : string.c_str() + length);
    }
    float text(const Vector2& pos, const std::string& string, size_t length = 0)
    {
        return text(pos.x, pos.y, string, length);
    }

    /** Draws a multi-line text box at the specified location, wrapped at `width`, up to `length` characters long.
     * White space is stripped at the beginning of the rows, the text is split at word boundaries or when new-line
     * characters are encountered.
     * Words longer than the max width are slit at nearest character (i.e. no hyphenation).
     * @param x         X-coordinate.
     * @param y         Y-coordinate.
     * @param width     Maximal width of a line in the text box.
     * @param string    Text to draw.
     * @param length    Number of characters to print (defaults to 0 = all).
     */
    void text_box(float x, float y, float width, const std::string& string, size_t length = 0)
    {
        nvgTextBox(_get_context(), x, y, width, string.c_str(), (length == 0) ? nullptr : string.c_str() + length);
    }
    void text_box(const Vector2& pos, float width, const std::string& string, size_t length = 0)
    {
        text_box(pos.x, pos.y, width, string, length);
    }
    void text_box(const Aabr& rect, const std::string& string, size_t length = 0)
    {
        text_box(rect.left(), rect.top(), rect.width(), string, length);
    }

    /** Returns the bounding box of the specified text in local space. */
    Aabr text_bounds(float x, float y, const std::string& string, size_t length = 0)
    {
        Aabr result;
        nvgTextBounds(_get_context(), x, y, string.c_str(), (length == 0) ? nullptr : string.c_str() + length, result.as_ptr());
        return result;
    }
    Aabr text_bounds(const Vector2& pos, const std::string& string, size_t length = 0)
    {
        return text_bounds(pos.x, pos.y, string, length);
    }
    Aabr text_bounds(const std::string& string, size_t length = 0)
    {
        return text_bounds(0.f, 0.f, string, length);
    }

    /** Returns the bounding box of the specified text box in local space. */
    Aabr text_box_bounds(float x, float y, float width, const std::string& string, size_t length = 0)
    {
        Aabr result;
        nvgTextBoxBounds(_get_context(), x, y, width, string.c_str(), (length == 0) ? nullptr : string.c_str() + length, result.as_ptr());
        return result;
    }
    Aabr text_box_bounds(const Vector2& pos, float width, const std::string& string, size_t length = 0)
    {
        return text_box_bounds(pos.x, pos.y, width, string, length);
    }
    Aabr text_box_bounds(const Aabr& rect, const std::string& string, size_t length = 0)
    {
        return text_box_bounds(rect.left(), rect.top(), rect.width(), string, length);
    }
    Aabr text_box_bounds(float width, const std::string& string, size_t length = 0)
    {
        return text_box_bounds(0.f, 0.f, width, string, length);
    }

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

    /** Height of the state stack created by this Painter. The first state is internal and may not be popped by the user. */
    size_t m_state_count;
};

} // namespace notf
