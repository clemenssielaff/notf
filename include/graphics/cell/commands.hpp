#pragma once

#include "graphics/cell/command_buffer.hpp"
#include "graphics/cell/painter.hpp"

namespace notf {

/** Command to copy the current PainterState and push it on the states stack. */
struct PushStateCommand : public PainterCommand {
    PushStateCommand()
        : PainterCommand(PUSH_STATE) {}
};

/** Command to remove the current PainterState and back to the previous one. */
struct PopStateCommand : public PainterCommand {
    PopStateCommand()
        : PainterCommand(POP_STATE) {}
};

/** Command to start a new path. */
struct BeginCommand : public PainterCommand {
    BeginCommand()
        : PainterCommand(BEGIN_PATH) {}
};

/** Command setting the winding direction for the next fill or stroke. */
struct SetWindingCommand : public PainterCommand {
    SetWindingCommand(Painter::Winding winding)
        : PainterCommand(SET_WINDING), winding(winding) {}
    Painter::Winding winding;
};

/** Command to close the current path. */
struct CloseCommand : public PainterCommand {
    CloseCommand()
        : PainterCommand(CLOSE) {}
};

/** Command to move the Painter's stylus without drawing a line.
 * Creates a new path.
 */
struct MoveCommand : public PainterCommand {
    MoveCommand(Vector2f pos)
        : PainterCommand(MOVE), pos(std::move(pos)) {}
    Vector2f pos;
};

/** Command to draw a line from the current stylus position to the one given. */
struct LineCommand : public PainterCommand {
    LineCommand(Vector2f pos)
        : PainterCommand(LINE), pos(std::move(pos)) {}
    Vector2f pos;
};

/** Command to draw a bezier spline from the current stylus position. */
struct BezierCommand : public PainterCommand {
    BezierCommand(Vector2f ctrl1, Vector2f ctrl2, Vector2f end)
        : PainterCommand(BEZIER), ctrl1(std::move(ctrl1)), ctrl2(std::move(ctrl2)), end(std::move(end)) {}
    Vector2f ctrl1;
    Vector2f ctrl2;
    Vector2f end;
};

/** Command to fill the current paths using the current PainterState. */
struct FillCommand : public PainterCommand {
    FillCommand()
        : PainterCommand(FILL) {}
};

/** Command to stroke the current paths using the current PainterState. */
struct StrokeCommand : public PainterCommand {
    StrokeCommand()
        : PainterCommand(STROKE) {}
};

/** Command to change the Xform of the current PainterState. */
struct SetXformCommand : public PainterCommand {
    SetXformCommand(Matrix3f xform)
        : PainterCommand(SET_XFORM), xform(std::move(xform)) {}
    Matrix3f xform;
};

/** Command to reset the Xform of the current PainterState. */
struct ResetXformCommand : public PainterCommand {
    ResetXformCommand()
        : PainterCommand(RESET_XFORM) {}
};

/** Command to transform the current Xform of the current PainterState. */
struct TransformCommand : public PainterCommand {
    TransformCommand(Matrix3f xform)
        : PainterCommand(TRANSFORM), xform(std::move(xform)) {}
    Matrix3f xform;
};

/** Command to add a translation the Xform of the current PainterState. */
struct TranslationCommand : public PainterCommand {
    TranslationCommand(Vector2f delta)
        : PainterCommand(TRANSLATE), delta(std::move(delta)) {}
    Vector2f delta;
};

/** Command to add a rotation in radians to the Xform of the current PainterState. */
struct RotationCommand : public PainterCommand {
    RotationCommand(float angle)
        : PainterCommand(ROTATE), angle(angle) {}
    float angle;
};

/** Command to set the Scissor of the current PainterState. */
struct SetScissorCommand : public PainterCommand {
    SetScissorCommand(Scissor sissor)
        : PainterCommand(SET_SCISSOR), scissor(sissor) {}
    Scissor scissor;
};

/** Command to reset the Scissor of the current PainterState. */
struct ResetScissorCommand : public PainterCommand {
    ResetScissorCommand()
        : PainterCommand(RESET_SCISSOR) {}
};

/** Command to set the fill Color of the current PainterState. */
struct FillColorCommand : public PainterCommand {
    FillColorCommand(Color color)
        : PainterCommand(SET_FILL_COLOR), color(color) {}
    Color color;
};

/** Command to set the fill Paint of the current PainterState. */
struct FillPaintCommand : public PainterCommand {
    FillPaintCommand(Paint paint)
        : PainterCommand(SET_FILL_PAINT), paint(paint) {}
    Paint paint;
};

/** Command to set the stroke Color of the current PainterState. */
struct StrokeColorCommand : public PainterCommand {
    StrokeColorCommand(Color color)
        : PainterCommand(SET_STROKE_COLOR), color(color) {}
    Color color;
};

/** Command to set the stroke Paint of the current PainterState. */
struct StrokePaintCommand : public PainterCommand {
    StrokePaintCommand(Paint paint)
        : PainterCommand(SET_STROKE_PAINT), paint(paint) {}
    Paint paint;
};

/** Command to set the stroke width of the current PainterState. */
struct StrokeWidthCommand : public PainterCommand {
    StrokeWidthCommand(float stroke_width)
        : PainterCommand(SET_STROKE_WIDTH), stroke_width(stroke_width) {}
    float stroke_width;
};

/** Command to set the BlendMode of the current PainterState. */
struct BlendModeCommand : public PainterCommand {
    BlendModeCommand(BlendMode blend_mode)
        : PainterCommand(SET_BLEND_MODE), blend_mode(blend_mode) {}
    BlendMode blend_mode;
};

/** Command to set the alpha of the current PainterState. */
struct SetAlphaCommand : public PainterCommand {
    SetAlphaCommand(float alpha)
        : PainterCommand(SET_ALPHA), alpha(alpha) {}
    float alpha;
};

/** Command to set the MiterLimit of the current PainterState. */
struct MiterLimitCommand : public PainterCommand {
    MiterLimitCommand(float miter_limit)
        : PainterCommand(SET_MITER_LIMIT), miter_limit(miter_limit) {}
    float miter_limit;
};

/** Command to set the LineCap of the current PainterState. */
struct LineCapCommand : public PainterCommand {
    LineCapCommand(Painter::LineCap line_cap)
        : PainterCommand(SET_LINE_CAP), line_cap(line_cap) {}
    Painter::LineCap line_cap;
};

/** Command to set the LineJoinof the current PainterState. */
struct LineJoinCommand : public PainterCommand {
    LineJoinCommand(Painter::LineJoin line_join)
        : PainterCommand(SET_LINE_JOIN), line_join(line_join) {}
    Painter::LineJoin line_join;
};

/** Command to render the given text in the given font. */
struct RenderTextCommand : public PainterCommand {
    RenderTextCommand(std::shared_ptr<std::string> text, std::shared_ptr<Font> font)
        : PainterCommand(RENDER_TEXT), text(text), font(font) {}
    std::shared_ptr<std::string> text;
    std::shared_ptr<Font> font;
};

} // namespace notf
