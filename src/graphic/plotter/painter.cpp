#include "notf/graphic/plotter/painter.hpp"

#include "notf/meta/log.hpp"

#include "notf/common/geo/segment.hpp"

#include "notf/graphic/plotter/design.hpp"

NOTF_OPEN_NAMESPACE

// painter ========================================================================================================== //

Painter::Painter(WidgetDesign& design) : m_design(design) { m_design.reset(); }

//Painter::PathId Painter::set_path(Polygonf polygon) {
//    // TODO: make sure that this turns into a `set_path_index` command should the path already exist on the painter
//    m_design.add_command<WidgetDesign::SetPolygonPathCommand>(std::move(polygon));
//    m_current_path_id = m_next_path_id++;
//    return m_current_path_id;
//}

//Painter::PathId Painter::set_path(CubicBezier2f spline) {
//    m_design.add_command<WidgetDesign::SetSplinePathCommand>(std::move(spline));
//    m_current_path_id = m_next_path_id++;
//    return m_current_path_id;
//}

//Painter::PathId Painter::set_path(PathId id) {
//    if (id >= m_next_path_id) {
//        // no change if the given ID is invalid
//        NOTF_LOG_WARN("Path ID \"{}\" is not a valid Path - using current one instead", id);
//    } else {
//        m_design.add_command<WidgetDesign::SetPathIndexCommand>(id);
//        m_current_path_id = id;
//    }
//    return m_current_path_id;
//}

void Painter::set_font(FontPtr font) {
    State& current_state = _get_current_state();
    if (font && current_state.font != font) {
        current_state.font = font;
        m_design.add_command<WidgetDesign::SetFontCommand>(std::move(font));
    }
}

void Painter::write(std::string text) { m_design.add_command<WidgetDesign::WriteCommand>(std::move(text)); }

void Painter::fill() { m_design.add_command<WidgetDesign::FillCommand>(); }

void Painter::stroke() { m_design.add_command<WidgetDesign::StrokeCommand>(); }

void Painter::set_transform(const M3f& transform) {
    if (!transform.is_identity()) {
        State& current_state = _get_current_state();
        current_state.xform = transform;
        m_design.add_command<WidgetDesign::SetTransformationCommand>(current_state.xform);
    }
}

void Painter::reset_transform() {
    State& current_state = _get_current_state();
    if (!current_state.xform.is_identity()) {
        current_state.xform = M3f::identity();
        m_design.add_command<WidgetDesign::SetTransformationCommand>(current_state.xform);
    }
}

void Painter::transform(const M3f& transform) {
    if (!transform.is_identity()) {
        State& current_state = _get_current_state();
        current_state.xform *= transform;
        m_design.add_command<WidgetDesign::SetTransformationCommand>(current_state.xform);
    }
}

void Painter::translate(const V2f& delta) {
    if (delta.get_magnitude_sq() > precision_low<float>()) {
        State& current_state = _get_current_state();
        current_state.xform.translate(delta);
        m_design.add_command<WidgetDesign::SetTransformationCommand>(current_state.xform);
    }
}

void Painter::rotate(const float angle) {
    if (!is_approx(angle, 0)) {
        State& current_state = _get_current_state();
        current_state.xform.rotate(angle);
        m_design.add_command<WidgetDesign::SetTransformationCommand>(current_state.xform);
    }
}

void Painter::set_clipping(Clipping clipping) {
    State& current_state = _get_current_state();
    if (current_state.clipping != clipping) {
        current_state.clipping = std::move(clipping);
        m_design.add_command<WidgetDesign::SetClippingCommand>(current_state.clipping);
    }
}

void Painter::remove_clipping() {
    State& current_state = _get_current_state();
    if (current_state.clipping != Clipping()) {
        current_state.clipping = Clipping();
        m_design.add_command<WidgetDesign::SetClippingCommand>(current_state.clipping);
    }
}

void Painter::set_blend_mode(const BlendMode mode) {
    State& current_state = _get_current_state();
    if (current_state.blend_mode != mode) {
        current_state.blend_mode = mode;
        m_design.add_command<WidgetDesign::SetBlendModeCommand>(current_state.blend_mode);
    }
}

void Painter::set_alpha(const float alpha) {
    State& current_state = _get_current_state();
    if (!is_approx(current_state.alpha, alpha)) {
        current_state.alpha = alpha;
        m_design.add_command<WidgetDesign::SetAlphaCommand>(current_state.alpha);
    }
}

void Painter::set_line_cap(const LineCap cap) {
    State& current_state = _get_current_state();
    if (current_state.line_cap != cap) {
        current_state.line_cap = cap;
        m_design.add_command<WidgetDesign::SetLineCapCommand>(current_state.line_cap);
    }
}

void Painter::set_line_join(const LineJoin join) {
    State& current_state = _get_current_state();
    if (current_state.line_join != join) {
        current_state.line_join = join;
        m_design.add_command<WidgetDesign::SetLineJoinCommand>(current_state.line_join);
    }
}

void Painter::set_fill(Paint paint) {
    State& current_state = _get_current_state();
    if (current_state.fill_paint != paint) {
        current_state.fill_paint = std::move(paint);
        m_design.add_command<WidgetDesign::SetFillPaintCommand>(current_state.fill_paint);
    }
}

void Painter::set_stroke(Paint paint) {
    State& current_state = _get_current_state();
    if (current_state.stroke_paint != paint) {
        current_state.stroke_paint = std::move(paint);
        m_design.add_command<WidgetDesign::SetStrokePaintCommand>(current_state.stroke_paint);
    }
}

void Painter::set_stroke_width(float width) {
    width = max(0, width);
    State& current_state = _get_current_state();
    if (!is_approx(current_state.stroke_width, width)) {
        current_state.stroke_width = width;
        m_design.add_command<WidgetDesign::SetStrokeWidthCommand>(current_state.stroke_width);
    }
}

NOTF_CLOSE_NAMESPACE
