#include "graphics/painter.hpp"

#include "common/color.hpp"
#include "core/widget.hpp"
#include "graphics/cell.hpp"
#include "graphics/render_context.hpp"

namespace notf {

void Painter::test()
{
    const float corner_radius = 20;
    const Size2f widget_size  = m_widget.get_size();
    const float margin        = 20;
    const Aabr base           = Aabr(margin, margin, widget_size.width - 2 * margin, widget_size.height - 2 * margin);

    auto gradient = m_cell.create_linear_gradient(Vector2(), Vector2(0, widget_size.height),
                                                  Color(0, 0, 0, 32), Color(255, 255, 255, 32));

    m_cell.begin_path();
    m_cell.set_fill_color(Color(0, 96, 128));
    m_cell.add_rounded_rect(base, corner_radius - 1);
    m_cell.fill(m_context);

    m_cell.begin_path();
    m_cell.add_rounded_rect(base, corner_radius - 1);
    m_cell.set_fill_paint(gradient);
    m_cell.fill(m_context);
}

} // namespace notf
