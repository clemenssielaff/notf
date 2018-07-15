#include "app/widget/painter.hpp"

#include "app/widget/design.hpp"
#include "common/log.hpp"
#include "common/segment.hpp"

NOTF_OPEN_NAMESPACE

// ================================================================================================================== //

Painter::Painter(WidgetDesign& design) : m_design(design) { m_design.reset(); }

Painter::PathId Painter::set_path(Polygonf polygon)
{
    m_design.add_command<WidgetDesign::SetPolygonPathCommand>(std::move(polygon));
    m_current_path_id = m_next_path_id++;
    return m_current_path_id;
}

Painter::PathId Painter::set_path(CubicBezier2f spline)
{
    m_design.add_command<WidgetDesign::SetSplinePathCommand>(std::move(spline));
    m_current_path_id = m_next_path_id++;
    return m_current_path_id;
}

Painter::PathId Painter::set_path(PathId id)
{
    if (id >= m_next_path_id) {
        // no change if the given ID is invalid
        log_warning << "Path ID \"" << id << "\" is not a valid Path - using current one instead";
    }
    else {
        m_design.add_command<WidgetDesign::SetPathIndexCommand>(id);
        m_current_path_id = id;
    }
    return m_current_path_id;
}

void Painter::write(std::string text, FontPtr font)
{
    m_design.add_command<WidgetDesign::WriteCommand>(std::move(text), std::move(font));
}

void Painter::fill() { m_design.add_command<WidgetDesign::FillCommand>(); }

void Painter::stroke() { m_design.add_command<WidgetDesign::StrokeCommand>(); }

void Painter::set_stroke_width(const float width)
{
    State& current_state = _get_current_state();
    current_state.stroke_width = max(0, width);
    m_design.add_command<WidgetDesign::SetStrokeWidthCommand>(current_state.stroke_width);
}

NOTF_CLOSE_NAMESPACE
