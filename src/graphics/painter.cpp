#include "graphics/painter.hpp"

#include "common/color.hpp"
#include "graphics/cell.hpp"

namespace notf {

void Painter::test()
{
    m_cell.begin_path();
    m_cell.set_fill_color(Color(0., 1., 0.));
    m_cell.add_rect(10, 20, 30, 40);
    m_cell.fill(m_context);
}

} // namespace notf
