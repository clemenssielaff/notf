#include "notf/app/widget/painter.hpp"

#include "notf/meta/log.hpp"

#include "notf/common/segment.hpp"

#include "notf/app/widget/widget_design.hpp"

NOTF_OPEN_NAMESPACE

// painter ========================================================================================================== //

Painter::Painter(WidgetDesign& design) : m_design(design) { m_design.reset(); }

Painter::PathId Painter::set_path(Polygonf polygon) {
    m_design.add_command<WidgetDesign::SetPolygonPathCommand>(std::move(polygon));
    m_current_path_id = m_next_path_id++;
    return m_current_path_id;
}

Painter::PathId Painter::set_path(CubicBezier2f spline) {
    m_design.add_command<WidgetDesign::SetSplinePathCommand>(std::move(spline));
    m_current_path_id = m_next_path_id++;
    return m_current_path_id;
}

Painter::PathId Painter::set_path(PathId id) {
    if (id >= m_next_path_id) {
        // no change if the given ID is invalid
        NOTF_LOG_WARN("Path ID \"{}\" is not a valid Path - using current one instead", id);
    } else {
        m_design.add_command<WidgetDesign::SetPathIndexCommand>(id);
        m_current_path_id = id;
    }
    return m_current_path_id;
}

void Painter::set_font(FontPtr font) {
    State& current_state = _get_current_state();
    if (!font || current_state.font == font) {
        return; // early out
    }
    current_state.font = font;
    m_design.add_command<WidgetDesign::SetFontCommand>(std::move(font));
}

void Painter::write(std::string text) { m_design.add_command<WidgetDesign::WriteCommand>(std::move(text)); }

void Painter::fill() { m_design.add_command<WidgetDesign::FillCommand>(); }

void Painter::stroke() { m_design.add_command<WidgetDesign::StrokeCommand>(); }

void Painter::set_stroke_width(const float width) {
    State& current_state = _get_current_state();
    current_state.stroke_width = max(0, width);
    m_design.add_command<WidgetDesign::SetStrokeWidthCommand>(current_state.stroke_width);
}

void Painter::set_transform(const M3f& xform) {
    State& current_state = _get_current_state();
    current_state.xform = xform;
    m_design.add_command<WidgetDesign::SetTransformationCommand>(current_state.xform);
}

void Painter::reset_transform() {
    State& current_state = _get_current_state();
    current_state.xform = M3f::identity();
    m_design.add_command<WidgetDesign::SetTransformationCommand>(current_state.xform);
}

void Painter::transform(const M3f& transform) {
    State& current_state = _get_current_state();
    current_state.xform *= transform;
    m_design.add_command<WidgetDesign::SetTransformationCommand>(current_state.xform);
}

void Painter::translate(const V2f& delta) {
    State& current_state = _get_current_state();
    current_state.xform.translate(delta);
    m_design.add_command<WidgetDesign::SetTransformationCommand>(current_state.xform);
}

void Painter::rotate(const float angle) {
    State& current_state = _get_current_state();
    current_state.xform.rotate(angle);
    m_design.add_command<WidgetDesign::SetTransformationCommand>(current_state.xform);
}

NOTF_CLOSE_NAMESPACE
