#include "graphics/stats.hpp"

#include "graphics/cell/cell_canvas.hpp"
#include "graphics/cell/painter.hpp"
#include "graphics/cell/painterpreter.hpp"

namespace notf {

RenderStats::RenderStats(const size_t buffer_size)
    : m_buffer(buffer_size, 0)
    , m_head(0)
    , m_max_frame_time(1000.f / 60.f)
    , m_aabr(10, 10, 200, 35)
{
    m_buffer.shrink_to_fit();
}

void RenderStats::update(const float frame_time)
{
    m_head           = (m_head + 1) % m_buffer.size();
    m_buffer[m_head] = frame_time;
}

float RenderStats::get_average() const
{
    size_t count = 0;
    float sum    = 0.f;
    for (size_t i = 0; i < m_buffer.size(); ++i) {
        if (m_buffer[i] > 0) {
            sum += m_buffer[i];
            ++count;
        }
    }
    if (count == 0) {
        return 0;
    }
    return sum / static_cast<float>(count);
}

void RenderStats::render_stats(CellCanvas& context)
{
    if (m_buffer.size() < 2) {
        return;
    }

    Painter painter(context, m_cell);

    painter.begin_path();
    painter.add_rect(m_aabr);
    painter.set_fill(Color(0, 0, 0, 128));
    painter.fill();

    /*   ++   +  +
     *   | +   ++ +    ++
     *   |  ++     +  + |
     *   |          ++  |
     *   |______________|
     */

    // the graph consists of many vertices on the top and two on the bottom
    // it is drawn from right to left so that the vertices form a counter-clockwise polygon
    painter.begin_path();
    painter.move_to(m_aabr.bottom_right());
    float y = m_aabr.bottom();
    for (size_t i = 0; i < m_buffer.size(); i++) {
        const float value = min(m_max_frame_time, m_buffer[(m_head - i) % m_buffer.size()] * 1000.0f);
        const float x     = m_aabr.right() - (static_cast<float>(i) / (m_buffer.size() - 1)) * m_aabr.get_width();
        y                 = m_aabr.bottom() + ((value / m_max_frame_time) * m_aabr.get_height());
        painter.line_to(x, y);
    }
    painter.line_to(m_aabr.left() - 1, y); // to avoid a pixel-wide gap on thr right edge of the stats box
    painter.line_to(m_aabr.bottom_left());
    painter.set_fill(Color(255, 192, 0, 128));
    painter.fill();

    context.paint(m_cell);
}

} // namespace notf
