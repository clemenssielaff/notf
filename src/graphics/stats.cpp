#include "graphics/stats.hpp"

namespace notf {

RenderStats::RenderStats(const size_t buffer_size)
    : m_buffer(buffer_size, 0)
    , m_head(0)
    , m_max_frame_time(1000.f/60.f)
    , m_aabr(10, 10, 200, 35)
{
    m_buffer.shrink_to_fit();
}

void RenderStats::update(const float frame_time)
{
    m_head = (m_head + 1) % m_buffer.size();
    m_buffer[m_head] = frame_time;
}

float RenderStats::get_average() const
{
    size_t count = 0;
    float sum = 0.f;
    for(size_t i = 0; i < m_buffer.size(); ++i){
        if(m_buffer[i] > 0){
            sum += m_buffer[i];
            ++count;
        }
    }
    if(count == 0){
        return 0;
    }
    return sum / static_cast<float>(count);
}

void RenderStats::render_stats(RenderContext &context)
{
    if(m_buffer.size() < 2){
        return;
    }

    m_cell.reset(context);

    m_cell.begin_path();
    m_cell.add_rect(m_aabr);
    m_cell.set_fill_color(Color(0, 0, 0, 128));
    m_cell.fill(context);

    /*   ++   +  +
     *   | +   ++ +    ++
     *   |  ++     +  + |
     *   |          ++  |
     *   |______________|
     */

    // the graph consists of many vertices on the top and two on the bottom
    m_cell.begin_path();
    m_cell.move_to(m_aabr.bottom_left());
    for (size_t i = 0; i < m_buffer.size(); i++) {
        const float value = min(m_max_frame_time, m_buffer[(m_head + i) % m_buffer.size()] * 1000.0f);
        const float x = m_aabr.left() + (static_cast<float>(i)/(m_buffer.size()-1)) * m_aabr.width();
        const float y = m_aabr.bottom() - ((value / m_max_frame_time) * m_aabr.height());
        m_cell.line_to(x, y);
    }
    m_cell.line_to(m_aabr.bottom_right());
    m_cell.set_fill_color(Color(255,192,0,128));
    m_cell.fill(context);
}

} // namespace notf
