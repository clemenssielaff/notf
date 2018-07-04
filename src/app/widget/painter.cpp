#include "app/widget/painter.hpp"

#include "app/widget/design.hpp"
#include "common/segment.hpp"

NOTF_OPEN_NAMESPACE

// ================================================================================================================== //

Painter::Painter(WidgetDesign& design) : m_design(design) { m_design.reset(); }

size_t Painter::push_state()
{
    m_design.add_command(WidgetDesign::PushStateCommand());
    m_states.emplace_back(m_states.back());
    return m_states.size();
}

size_t Painter::pop_state()
{
    if (m_states.size() > 1) {
        m_design.add_command(WidgetDesign::PopStateCommand());
        m_states.pop_back();
    }
    return m_states.size();
}

void Painter::set_transform(const Matrix3f& xform)
{
    State& current_state = _get_current_state();
    current_state.xform = xform;
    m_design.add_command(WidgetDesign::SetTransformCommand(current_state.xform));
}

void Painter::reset_transform()
{
    _get_current_state().xform = Matrix3f::identity();
    m_design.add_command(WidgetDesign::ResetTransformCommand());
}

void Painter::transform(const Matrix3f& transform)
{
    m_design.add_command(WidgetDesign::TransformCommand(transform));
    _get_current_state().xform.premult(transform);
}

void Painter::translate(const Vector2f& delta)
{
    m_design.add_command(WidgetDesign::TranslationCommand(delta));
    _get_current_state().xform.translate(delta);
}

void Painter::rotate(const float angle)
{
    m_design.add_command(WidgetDesign::RotationCommand(angle));
    _get_current_state().xform.rotate(angle);
}

void Painter::set_clipping(Clipping clipping)
{
    State& current_state = _get_current_state();
    current_state.clipping = clipping;
    m_design.add_command(WidgetDesign::SetClippingCommand(std::move(clipping)));
}

void Painter::remove_clipping()
{
    m_design.add_command(WidgetDesign::ResetClippingCommand());
    _get_current_state().clipping = Clipping();
}

void Painter::set_blend_mode(const BlendMode mode)
{
    m_design.add_command(WidgetDesign::BlendModeCommand(mode));
    _get_current_state().blend_mode = mode;
}

void Painter::set_alpha(const float alpha)
{
    m_design.add_command(WidgetDesign::SetAlphaCommand(alpha));
    _get_current_state().alpha = alpha;
}

void Painter::set_miter_limit(const float limit)
{
    m_design.add_command(WidgetDesign::MiterLimitCommand(limit));
    _get_current_state().miter_limit = limit;
}

void Painter::set_line_cap(const Paint::LineCap cap)
{
    m_design.add_command(WidgetDesign::LineCapCommand(cap));
    _get_current_state().line_cap = cap;
}

void Painter::set_line_join(const Paint::LineJoin join)
{
    m_design.add_command(WidgetDesign::LineJoinCommand(join));
    _get_current_state().line_join = join;
}

void Painter::set_fill(Paint paint)
{
    State& current_state = _get_current_state();
    paint.xform.premult(current_state.xform);
    current_state.fill_paint = std::move(paint);
    // TODO: make sure that the correct overload (the one with the vault) is called (also in set_stroke(Paint))
    m_design.add_command(WidgetDesign::FillPaintCommand(current_state.fill_paint));
}

void Painter::set_fill(Color color)
{
    m_design.add_command(WidgetDesign::FillColorCommand(color));
    State& current_state = _get_current_state();
    current_state.fill_paint.set_color(std::move(color));
}

void Painter::set_stroke(Paint paint)
{
    State& current_state = _get_current_state();
    paint.xform.premult(current_state.xform);
    current_state.stroke_paint = std::move(paint);
    m_design.add_command(WidgetDesign::StrokePaintCommand(current_state.stroke_paint));
}

void Painter::set_stroke(Color color)
{
    m_design.add_command(WidgetDesign::StrokeColorCommand(color));
    State& current_state = _get_current_state();
    current_state.stroke_paint.set_color(std::move(color));
}

void Painter::set_stroke_width(const float width)
{
    State& current_state = _get_current_state();
    current_state.stroke_width = max(0, width);
    m_design.add_command(WidgetDesign::StrokeWidthCommand(current_state.stroke_width));
}

void Painter::begin_path() { m_design.add_command(WidgetDesign::BeginPathCommand()); }

void Painter::close_path()
{
    m_design.add_command(WidgetDesign::ClosePathCommand());
    m_has_open_path = false;
}

void Painter::set_winding(const Paint::Winding winding)
{
    m_design.add_command(WidgetDesign::SetWindingCommand(winding));
}

void Painter::move_to(Vector2f pos) { m_design.add_command(WidgetDesign::MoveCommand(std::move(pos))); }

void Painter::line_to(Vector2f pos)
{
    m_design.add_command(WidgetDesign::LineCommand(std::move(pos)));
    m_has_open_path = true;
}

void Painter::quad_to(const float cx, const float cy, const float tx, const float ty)
{
    // convert the quad spline into a bezier
    m_design.add_command(WidgetDesign::BezierCommand({m_stylus.x() + (2.f / 3.f) * (cx - m_stylus.x()),  // ctrl1 x
                                                      m_stylus.y() + (2.f / 3.f) * (cy - m_stylus.y())}, // ctrl1 y
                                                     {tx + (2.f / 3.f) * (cx - tx),                      // ctrl2 x
                                                      ty + (2.f / 3.f) * (cy - ty)},                     // ctrl2 y
                                                     {tx, ty}));                                         // end
    m_has_open_path = true;
}

void Painter::bezier_to(Vector2f ctrl1, Vector2f ctrl2, Vector2f end)
{
    m_design.add_command(WidgetDesign::BezierCommand(std::move(ctrl1), std::move(ctrl2), std::move(end)));
    m_has_open_path = true;
}

void Painter::arc(const float cx, const float cy, const float r, const float a0, const float a1,
                  const Paint::Winding dir)
{
    // clamp angles
    float da;
    if (dir == Paint::Winding::CLOCKWISE) {
        da = norm_angle(a1 - a0);
    }
    else {
        assert(dir == Paint::Winding::COUNTERCLOCKWISE);
        da = -norm_angle(a0 - a1);
    }
    // split the arc into <= 90deg segments
    const float ndivs = max(1, min(ceilf(abs(da) / (pi<float>() / 2)), 5));
    const float hda = (da / ndivs) / 2;
    const float kappa = abs(4.f / 3.f * (1.f - cos(hda)) / sin(hda)) * (dir == Paint::Winding::CLOCKWISE ? 1 : -1);

    // create individual commands
    float px = 0, py = 0, ptanx = 0, ptany = 0;
    for (float i = 0; i <= ndivs; i++) {
        const float a = a0 + da * (i / ndivs);
        const float dx = cos(a);
        const float dy = sin(a);
        const float x = cx + dx * r;
        const float y = cy + dy * r;
        const float tanx = -dy * r * kappa;
        const float tany = dx * r * kappa;
        if (static_cast<int>(i) == 0) {
            if (!m_has_open_path) { // TODO: this is the only time this variable is used - is it really necessary?
                                    //       ... or should it be used even more often?
                m_design.add_command(WidgetDesign::MoveCommand({x, y}));
            }
            else {
                m_design.add_command(WidgetDesign::LineCommand({x, y}));
            }
        }
        else {
            m_design.add_command(WidgetDesign::BezierCommand({px + ptanx, py + ptany}, {x - tanx, y - tany}, {x, y}));
        }
        px = x;
        py = y;
        ptanx = tanx;
        ptany = tany;
    }
    m_has_open_path = true;
}

void Painter::arc_to(const Vector2f& tangent, const Vector2f& end, const float radius)
{
    // handle degenerate cases
    const float distance_tolerance = _get_current_state().distance_tolerance;
    if (radius < distance_tolerance || m_stylus.is_approx(tangent, distance_tolerance)
        || tangent.is_approx(end, distance_tolerance)
        || Segment2f(m_stylus, end).get_closest_point(tangent).magnitude_sq()
               < (distance_tolerance * distance_tolerance)) {
        return line_to(end);
    }

    // calculate tangential circle to lines (stylus -> tangent) and (tangent -> end)
    const Vector2f delta1 = (m_stylus - tangent).normalize();
    const Vector2f delta2 = (tangent - end).normalize();
    const float a = acos(delta1.dot(delta2)) / 2;
    if (is_approx(a, 0)) {
        return line_to(end);
    }
    const float d = radius / tan(a);
    if (d > 10000.0f) { // I guess any large number would do
        return line_to(end);
    }

    // prepare the call to `arc` from the known arguments
    float cx, cy, a0, a1;
    Paint::Winding dir;
    if (delta1.cross(delta2) < 0) {
        cx = tangent.x() + delta1.x() * d + delta1.y() * radius;
        cy = tangent.y() + delta1.y() * d + -delta1.x() * radius;
        a0 = atan2(delta1.x(), -delta1.y());
        a1 = atan2(-delta2.x(), delta2.y());
        dir = Paint::Winding::CLOCKWISE;
    }
    else {
        cx = tangent.x() + delta1.x() * d + -delta1.y() * radius;
        cy = tangent.y() + delta1.y() * d + delta1.x() * radius;
        a0 = atan2(-delta1.x(), delta1.y());
        a1 = atan2(delta2.x(), -delta2.y());
        dir = Paint::Winding::COUNTERCLOCKWISE;
    }
    arc(cx, cy, radius, a0, a1, dir);
}

void Painter::add_rect(const float x, const float y, const float w, const float h)
{
    m_design.add_command(WidgetDesign::MoveCommand({x, y}));
    m_design.add_command(WidgetDesign::LineCommand({x + w, y}));
    m_design.add_command(WidgetDesign::LineCommand({x + w, y + h}));
    m_design.add_command(WidgetDesign::LineCommand({x, y + h}));
    close_path();
}

void Painter::add_rounded_rect(const float x, const float y, const float w, const float h, const float rtl,
                               const float rtr, const float rbr, const float rbl)
{
    if (rtl < 0.1f && rtr < 0.1f && rbr < 0.1f && rbl < 0.1f) {
        add_rect(std::move(x), std::move(y), std::move(w), std::move(h));
        return;
    }
    const float halfw = abs(w) * 0.5f;
    const float halfh = abs(h) * 0.5f;
    const float rxBL = min(rbl, halfw) * sign(w), ryBL = min(rbl, halfh) * sign(h);
    const float rxBR = min(rbr, halfw) * sign(w), ryBR = min(rbr, halfh) * sign(h);
    const float rxTR = min(rtr, halfw) * sign(w), ryTR = min(rtr, halfh) * sign(h);
    const float rxTL = min(rtl, halfw) * sign(w), ryTL = min(rtl, halfh) * sign(h);
    m_design.add_command(WidgetDesign::MoveCommand({x, y + ryTL}));
    m_design.add_command(WidgetDesign::LineCommand({x, y + h - ryBL}));
    m_design.add_command(WidgetDesign::BezierCommand({x, y + h - ryBL * (1 - kappa<float>())},
                                                     {x + rxBL * (1 - kappa<float>()), y + h}, {x + rxBL, y + h}));
    m_design.add_command(WidgetDesign::LineCommand({x + w - rxBR, y + h}));
    m_design.add_command(WidgetDesign::BezierCommand({x + w - rxBR * (1 - kappa<float>()), y + h},
                                                     {x + w, y + h - ryBR * (1 - kappa<float>())},
                                                     {x + w, y + h - ryBR}));
    m_design.add_command(WidgetDesign::LineCommand({x + w, y + ryTR}));
    m_design.add_command(WidgetDesign::BezierCommand({x + w, y + ryTR * (1 - kappa<float>())},
                                                     {x + w - rxTR * (1 - kappa<float>()), y}, {x + w - rxTR, y}));
    m_design.add_command(WidgetDesign::LineCommand({x + rxTL, y}));
    m_design.add_command(WidgetDesign::BezierCommand({x + rxTL * (1 - kappa<float>()), y},
                                                     {x, y + ryTL * (1 - kappa<float>())}, {x, y + ryTL}));
    close_path();
}

void Painter::add_ellipse(const float cx, const float cy, const float rx, const float ry)
{
    m_design.add_command(WidgetDesign::MoveCommand({cx - rx, cy}));
    m_design.add_command(WidgetDesign::BezierCommand({cx - rx, cy - ry * kappa<float>()},
                                                     {cx - rx * kappa<float>(), cy - ry}, {cx, cy - ry}));
    m_design.add_command(WidgetDesign::BezierCommand({cx + rx * kappa<float>(), cy - ry},
                                                     {cx + rx, cy - ry * kappa<float>()}, {cx + rx, cy}));
    m_design.add_command(WidgetDesign::BezierCommand({cx + rx, cy + ry * kappa<float>()},
                                                     {cx + rx * kappa<float>(), cy + ry}, {cx, cy + ry}));
    m_design.add_command(WidgetDesign::BezierCommand({cx - rx * kappa<float>(), cy + ry},
                                                     {cx - rx, cy + ry * kappa<float>()}, {cx - rx, cy}));
    close_path();
}

void Painter::write(const std::string& text, const std::shared_ptr<Font> font)
{
    m_design.add_command(WidgetDesign::RenderTextCommand(std::make_shared<std::string>(text), font));
}

void Painter::fill() { m_design.add_command(WidgetDesign::FillCommand()); }

void Painter::stroke() { m_design.add_command(WidgetDesign::StrokeCommand()); }

NOTF_CLOSE_NAMESPACE
