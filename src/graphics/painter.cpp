#include "graphics/painter.hpp"

#include "common/color.hpp"
#include "core/widget.hpp"
#include "graphics/cell.hpp"
#include "graphics/render_context.hpp"

#include "common/aabr.hpp"
#include "common/vector2.hpp"
#include "graphics/stats.hpp"

namespace notf {

void Painter::test()
{
    const float corner_radius = 200;
    const Size2f widget_size  = m_widget.get_size();
    const float margin        = 20;
    const Aabr base           = Aabr(margin, margin, widget_size.width - 2 * margin, widget_size.height - 2 * margin);

    //    auto gradient = m_cell.create_linear_gradient(Vector2(), Vector2(0, widget_size.height),
    //                                                  Color(0, 0, 0, 32), Color(255, 255, 255, 32));

    //    m_cell.begin_path();
    //    m_cell.set_fill_color(Color(0, 96, 128));
    //    m_cell.add_rounded_rect(base, corner_radius - 1);
    //    m_cell.fill(m_context);

    //    m_cell.begin_path();
    //    m_cell.add_rounded_rect(base, corner_radius - 1);
    //    m_cell.set_fill_paint(gradient);
    //    m_cell.fill(m_context);

    drawColorwheel(base, static_cast<float>(m_context.get_time().in_seconds()));
}

void Painter::drawColorwheel(const Aabr& rect, float t)
{
    float hue = sinf(t * 0.12f);

    m_cell.push_state();

    const float outer_radius = rect.shorter_size() * 0.5f - 5.0f;
    const float inner_radius = outer_radius - 20.0f;
    const float aeps         = 0.5f / outer_radius; // half a pixel arc length in radians (2pi cancels out).

    // hue circle
    for (int i = 0; i < 6; i++) {
        float a0 = static_cast<float>(i) / 6.0f * PI * 2.0f - aeps;
        float a1 = static_cast<float>(i + 1) / 6.0f * PI * 2.0f + aeps;
        m_cell.begin_path();
        m_cell.arc(rect.x(), rect.y(), inner_radius, a0, a1, Cell::Winding::CLOCKWISE);
        m_cell.arc(rect.x(), rect.y(), outer_radius, a1, a0, Cell::Winding::COUNTERCLOCKWISE);
        m_cell.close_path();

        const Vector2 start_pos{rect.x() + cosf(a0) * (inner_radius + outer_radius) * 0.5f, rect.y() + sinf(a0) * (inner_radius + outer_radius) * 0.5f};
        const Vector2 end_pos{rect.x() + cosf(a1) * (inner_radius + outer_radius) * 0.5f, rect.y() + sinf(a1) * (inner_radius + outer_radius) * 0.5f};
        const Color start_color = Color::from_hsl(360.f * a0 / (PI * 2), 1.0f, 0.55f, 1);
        const Color end_color   = Color::from_hsl(360.f * a1 / (PI * 2), 1.0f, 0.55f, 1);
        m_cell.set_fill_paint(Cell::create_linear_gradient(start_pos, end_pos, start_color, end_color));
        m_cell.fill(m_context);
    }

    // border around hue circle
    m_cell.begin_path();
    m_cell.add_circle(rect.x(), rect.y(), inner_radius - 0.5f);
    m_cell.add_circle(rect.x(), rect.y(), outer_radius + 0.5f);
    m_cell.set_stroke_color(Color(0, 0, 0, 64));
    m_cell.set_stroke_width(1.0f);
    m_cell.stroke(m_context);

    // Selector
    m_cell.push_state();
    m_cell.translate(rect.center());
    m_cell.rotate(hue * PI * 2);

    // marker on circle
    m_cell.begin_path();
    m_cell.add_rect(inner_radius - 2, -3, outer_radius - inner_radius + 4, 6);
    m_cell.set_stroke_width(2.0f);
    m_cell.set_stroke_color(Color(255, 255, 255, 192));
    m_cell.stroke(m_context);

    m_cell.begin_path();
    m_cell.add_rect(inner_radius - 2 - 10, -4 - 10, outer_radius - inner_radius + 4 + 20, 8 + 20);
    m_cell.add_rect(inner_radius - 2, -4, outer_radius - inner_radius + 4, 8);
    m_cell.set_winding(Cell::Winding::HOLE);
    const Vector2 center{inner_radius - 3, -5};
    const Size2f extend{outer_radius - inner_radius + 6, 10};
    m_cell.set_fill_paint(Cell::create_box_gradient(center, extend, 2, 4, Color(0, 0, 0, .5), Color(0, 0, 0, 0)));
    m_cell.fill(m_context);

    const float r = inner_radius - 6;

    // center triangle
    {
        const float ax = cosf(120.0f / 180.0f * PI) * r;
        const float ay = sinf(120.0f / 180.0f * PI) * r;
        const float bx = cosf(-120.0f / 180.0f * PI) * r;
        const float by = sinf(-120.0f / 180.0f * PI) * r;
        m_cell.begin_path();
        m_cell.move_to(r, 0);
        m_cell.line_to(ax, ay);
        m_cell.line_to(bx, by);
        m_cell.close_path();

        Vector2 start_pos{r, 0};
        Vector2 end_pos{ax, ay};
        Color start_color = Color::from_hsl(360.f *hue, 1.0f, 0.5f, 1);
        Color end_color   = Color(1, 1, 1, 1.);
        m_cell.set_fill_paint(Cell::create_linear_gradient(start_pos, end_pos, start_color, end_color));
        m_cell.fill(m_context);

        start_pos   = Vector2{(r + ax) * 0.5f, (0 + ay) * 0.5f};
        end_pos     = Vector2{bx, by};
        start_color = Color(0, 0, 0, 0.);
        end_color   = Color(0, 0, 0, 1.);
        m_cell.set_fill_paint(Cell::create_linear_gradient(start_pos, end_pos, start_color, end_color));
        m_cell.fill(m_context);
        m_cell.set_stroke_color(Color(0, 0, 0, 64));
        m_cell.stroke(m_context);
    }

    // Select circle on triangle
    const float ax = cosf(120.0f / 180.0f * PI) * r * 0.3f;
    const float ay = sinf(120.0f / 180.0f * PI) * r * 0.4f;
    m_cell.set_stroke_width(2);
    m_cell.begin_path();
    m_cell.add_circle(ax, ay, 5);
    m_cell.set_stroke_color(Color(1, 1, 1, 192));
    m_cell.stroke(m_context);

    m_cell.begin_path();
    m_cell.add_rect(ax - 20, ay - 20, 40, 40);
    m_cell.add_circle(ax, ay, 7);
    m_cell.set_winding(Cell::Winding::HOLE);
    m_cell.set_fill_paint(Cell::create_radial_gradient(Vector2{ax, ay}, 7, 9, Color(0, 0, 0, 64), Color(0, 0, 0, 0)));
    m_cell.fill(m_context);

    m_cell.pop_state();

    m_cell.pop_state();
}

} // namespace notf
