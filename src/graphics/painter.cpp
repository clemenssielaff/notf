#include "graphics/painter.hpp"

#include "common/color.hpp"
#include "core/widget.hpp"
#include "graphics/cell.hpp"
#include "graphics/render_context.hpp"

#include "common/aabr.hpp"
#include "common/log.hpp"
#include "common/vector2.hpp"
#include "graphics/stats.hpp"

namespace notf {

std::shared_ptr<Texture2> Painter::test_texture = {};

void Painter::test()
{
    const Size2f widget_size = m_widget.get_size();
    const Aabrf base(widget_size);
    const float margin = 20;
    const float time   = static_cast<float>(m_context.get_time().in_seconds());

    drawGraph(base, time);

    drawColorwheel(base.shrunken(margin), time);

    drawCheckBox(Aabrf{10, 100, 20, 20});

    drawButton(Aabrf{10, 130, 150, 30});

    drawSlider(Aabrf{10, 170, 150, 30}, 0.4f);

    drawCaps(Vector2f{10, 200}, 30);

    drawEyes(Aabrf{600, 20, 80, 60}, m_context.get_mouse_pos(), time);

    drawSpinner(base.center(), 100, time);

    drawJoins(Aabrf{120, widget_size.height - 50, 600, 50}, time);

    drawTexture(Aabrf{400, 100, 200, 200});
}

void Painter::drawSlider(const Aabrf& rect, float pos)
{
    float kr = (int)(rect.height() * 0.25f);

    m_cell.push_state();

    // Slot
    Paint bg = Cell::create_box_gradient(Vector2f{rect.left(), rect.y() - 2 + 1},
                                         Size2f{rect.width(), 4}, 2, 2, Color(0, 0, 0, 32), Color(0, 0, 0, 128));
    m_cell.begin_path();
    m_cell.add_rounded_rect(rect.left(), rect.y() - 2, rect.width(), 4, 2);
    m_cell.set_fill_paint(bg);
    m_cell.fill(m_context);

    // Knob Shadow
    bg = Cell::create_radial_gradient(Vector2f{rect.left() + (int)(pos * rect.width()), rect.y() + 1},
                                      kr - 3, kr + 3, Color(0, 0, 0, 64), Color(0, 0, 0, 0));
    m_cell.begin_path();
    m_cell.add_rect(rect.left() + (int)(pos * rect.width()) - kr - 5, rect.y() - kr - 5, kr * 2 + 5 + 5, kr * 2 + 5 + 5 + 3);
    m_cell.add_circle(rect.left() + (int)(pos * rect.width()), rect.y(), kr);
    m_cell.set_winding(Cell::Winding::HOLE);
    m_cell.set_fill_paint(bg);
    m_cell.fill(m_context);

    // Knob
    Paint knob = Cell::create_linear_gradient(Vector2f{rect.left(), rect.y() - kr},
                                              Vector2f{rect.left(), rect.y() + kr},
                                              Color(255, 255, 255, 16), Color(0, 0, 0, 16));
    m_cell.begin_path();
    m_cell.add_circle(rect.left() + (int)(pos * rect.width()), rect.y(), kr - 1);
    m_cell.set_fill_color(Color(40, 43, 48, 255));
    m_cell.fill(m_context);
    m_cell.set_fill_paint(knob);
    m_cell.fill(m_context);

    m_cell.begin_path();
    m_cell.add_circle(rect.left() + (int)(pos * rect.width()), rect.y(), kr - 0.5f);
    m_cell.set_stroke_color(Color(0, 0, 0, 92));
    m_cell.stroke(m_context);

    m_cell.pop_state();
}

void Painter::drawButton(const Aabrf& rect)
{
    const float corner_radius = 4;
    Paint gradient            = m_cell.create_linear_gradient(rect.top_left(), rect.bottom_left(),
                                                   Color(0, 0, 0, 32), Color(255, 255, 255, 32));

    m_cell.begin_path();
    m_cell.set_fill_color(Color(0, 96, 128));
    m_cell.add_rounded_rect(rect, corner_radius - 1);
    m_cell.fill(m_context);

    m_cell.begin_path();
    m_cell.add_rounded_rect(rect, corner_radius - 1);
    m_cell.set_fill_paint(gradient);
    m_cell.fill(m_context);
}

void Painter::drawCheckBox(const Aabrf& rect)
{
    Paint gradient = Cell::create_box_gradient(Vector2f{rect.left() + 1, rect.y() - 8},
                                               Size2f{18, 18}, 3, 3, Color(0, 0, 0, 32), Color(0, 0, 0, 92));
    m_cell.begin_path();
    m_cell.add_rounded_rect(rect.left() + 1, rect.y() - 9, 18, 18, 3);
    m_cell.set_fill_paint(gradient);
    m_cell.fill(m_context);
}

void Painter::drawColorwheel(const Aabrf& rect, float t)
{
    float hue = sin(t * 0.12f);

    m_cell.push_state();

    const float outer_radius = rect.shorter_size() * 0.5f - 5.0f;
    const float inner_radius = outer_radius - 20.0f;
    if(inner_radius <= 0){
        return;
    }
    const float aeps         = 0.5f / outer_radius; // half a pixel arc length in radians (2pi cancels out).

    // hue circle
    for (int i = 0; i < 6; i++) {
        float a0 = static_cast<float>(i) / 6.0f * TWO_PI - aeps;
        float a1 = static_cast<float>(i + 1) / 6.0f * TWO_PI + aeps;
        m_cell.begin_path();
        m_cell.arc(rect.x(), rect.y(), inner_radius, a0, a1, Cell::Winding::CLOCKWISE);
        m_cell.arc(rect.x(), rect.y(), outer_radius, a1, a0, Cell::Winding::COUNTERCLOCKWISE);
        m_cell.close_path();

        const Vector2f start_pos{rect.x() + cosf(a0) * (inner_radius + outer_radius) * 0.5f, rect.y() + sinf(a0) * (inner_radius + outer_radius) * 0.5f};
        const Vector2f end_pos{rect.x() + cosf(a1) * (inner_radius + outer_radius) * 0.5f, rect.y() + sinf(a1) * (inner_radius + outer_radius) * 0.5f};
        const Color start_color = Color::from_hsl(a0, 1.0f, 0.55f, 1);
        const Color end_color   = Color::from_hsl(a1, 1.0f, 0.55f, 1);
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
    m_cell.rotate(hue * TWO_PI);

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
    const Vector2f center{inner_radius - 3, -5};
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

        Vector2f start_pos{r, 0};
        Vector2f end_pos{ax, ay};
        Color start_color = Color::from_hsl(hue * TWO_PI, 1.0f, 0.5f, 1);
        Color end_color   = Color(1, 1, 1, 1.);
        m_cell.set_fill_paint(Cell::create_linear_gradient(start_pos, end_pos, start_color, end_color));
        m_cell.fill(m_context);

        start_pos   = Vector2f{(r + ax) * 0.5f, (0 + ay) * 0.5f};
        end_pos     = Vector2f{bx, by};
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
    m_cell.set_fill_paint(Cell::create_radial_gradient(Vector2f{ax, ay}, 7, 9, Color(0, 0, 0, 64), Color(0, 0, 0, 0)));
    m_cell.fill(m_context);

    m_cell.pop_state();

    m_cell.pop_state();
}

void Painter::drawEyes(const Aabrf& rect, const Vector2f& target, float t)
{
    float ex = rect.width() * 0.23f;
    float ey = rect.height() * 0.5f;
    float lx = rect.left() + ex;
    float ly = rect.top() + ey;
    float rx = rect.left() + rect.width() - ex;
    float ry = rect.top() + ey;
    float dx, dy, d;
    float br    = (ex < ey ? ex : ey) * 0.5f;
    float blink = 1 - pow(sinf(t * 0.5f), 200) * 0.8f;

    Paint bg = Cell::create_linear_gradient(Vector2f{rect.left(), rect.top() + rect.height() * 0.5f},
                                            Vector2f{rect.left() + rect.width() * 0.1f, rect.top() + rect.height()},
                                            Color(0, 0, 0, 32), Color(0, 0, 0, 16));
    m_cell.begin_path();
    m_cell.add_ellipse(lx + 3.0f, ly + 16.0f, ex, ey);
    m_cell.add_ellipse(rx + 3.0f, ry + 16.0f, ex, ey);
    m_cell.set_fill_paint(bg);
    m_cell.fill(m_context);

    bg = Cell::create_linear_gradient({rect.left(), rect.top() + rect.height() * 0.25f},
                                      {rect.left() + rect.width() * 0.1f, rect.top() + rect.height()}, Color(220, 220, 220, 255), Color(128, 128, 128, 255));
    m_cell.begin_path();
    m_cell.add_ellipse(lx, ly, ex, ey);
    m_cell.add_ellipse(rx, ry, ex, ey);
    m_cell.set_fill_paint(bg);
    m_cell.fill(m_context);

    dx = (target.x - rx) / (ex * 10);
    dy = (target.y - ry) / (ey * 10);
    d  = sqrtf(dx * dx + dy * dy);
    if (d > 1.0f) {
        dx /= d;
        dy /= d;
    }
    dx *= ex * 0.4f;
    dy *= ey * 0.5f;
    m_cell.begin_path();
    m_cell.add_ellipse(lx + dx, ly + dy + ey * 0.25f * (1 - blink), br, br * blink);
    m_cell.set_fill_color(Color(32, 32, 32, 255));
    m_cell.fill(m_context);

    dx = (target.x - rx) / (ex * 10);
    dy = (target.y - ry) / (ey * 10);
    d  = sqrtf(dx * dx + dy * dy);
    if (d > 1.0f) {
        dx /= d;
        dy /= d;
    }
    dx *= ex * 0.4f;
    dy *= ey * 0.5f;
    m_cell.begin_path();
    m_cell.add_ellipse(rx + dx, ry + dy + ey * 0.25f * (1 - blink), br, br * blink);
    m_cell.set_fill_color(Color(32, 32, 32, 255));
    m_cell.fill(m_context);

    Paint gloss = Cell::create_radial_gradient(Vector2f{lx - ex * 0.25f, ly - ey * 0.5f}, ex * 0.1f, ex * 0.75f, Color(255, 255, 255, 128), Color(255, 255, 255, 0));
    m_cell.begin_path();
    m_cell.add_ellipse(lx, ly, ex, ey);
    m_cell.set_fill_paint(gloss);
    m_cell.fill(m_context);

    gloss = Cell::create_radial_gradient(Vector2f{rx - ex * 0.25f, ry - ey * 0.5f}, ex * 0.1f, ex * 0.75f, Color(255, 255, 255, 128), Color(255, 255, 255, 0));
    m_cell.begin_path();
    m_cell.add_ellipse(rx, ry, ex, ey);
    m_cell.set_fill_paint(gloss);
    m_cell.fill(m_context);
}

void Painter::drawGraph(const Aabrf& rect, float t)
{
    float samples[6];
    float sx[6], sy[6];
    float dx = rect.width() / 5.0f;
    int i;

    samples[0] = (1 + sinf(t * 1.2345f + cosf(t * 0.33457f) * 0.44f)) * 0.5f;
    samples[1] = (1 + sinf(t * 0.68363f + cosf(t * 1.3f) * 1.55f)) * 0.5f;
    samples[2] = (1 + sinf(t * 1.1642f + cosf(t * 0.33457) * 1.24f)) * 0.5f;
    samples[3] = (1 + sinf(t * 0.56345f + cosf(t * 1.63f) * 0.14f)) * 0.5f;
    samples[4] = (1 + sinf(t * 1.6245f + cosf(t * 0.254f) * 0.3f)) * 0.5f;
    samples[5] = (1 + sinf(t * 0.345f + cosf(t * 0.03f) * 0.6f)) * 0.5f;

    for (i = 0; i < 6; i++) {
        sx[i] = rect.left() + i * dx;
        sy[i] = rect.top() + rect.height() * samples[i] * 0.8f;
    }

    // Graph background
    Paint bg = Cell::create_linear_gradient(rect.top_left(), rect.bottom_left(), Color(0, 160, 192, 0), Color(0, 160, 192, 64));
    m_cell.begin_path();
    m_cell.move_to(sx[0], sy[0]);
    for (i = 1; i < 6; i++)
        m_cell.bezier_to(sx[i - 1] + dx * 0.5f, sy[i - 1], sx[i] - dx * 0.5f, sy[i], sx[i], sy[i]);
    m_cell.line_to(rect.left() + rect.width(), rect.top() + rect.height());
    m_cell.line_to(rect.left(), rect.top() + rect.height());
    m_cell.set_fill_paint(bg);
    m_cell.fill(m_context);

    // Graph line
    m_cell.begin_path();
    m_cell.move_to(sx[0], sy[0] + 2);
    for (i = 1; i < 6; i++)
        m_cell.bezier_to(sx[i - 1] + dx * 0.5f, sy[i - 1] + 2, sx[i] - dx * 0.5f, sy[i] + 2, sx[i], sy[i] + 2);
    m_cell.set_stroke_color(Color(0, 0, 0, 32));
    m_cell.set_stroke_width(3.0f);
    m_cell.stroke(m_context);

    m_cell.begin_path();
    m_cell.move_to(sx[0], sy[0]);
    for (i = 1; i < 6; i++)
        m_cell.bezier_to(sx[i - 1] + dx * 0.5f, sy[i - 1], sx[i] - dx * 0.5f, sy[i], sx[i], sy[i]);
    m_cell.set_stroke_color(Color(0, 160, 192, 255));
    m_cell.set_stroke_width(3.0f);
    m_cell.stroke(m_context);

    // Graph sample pos
    for (i = 0; i < 6; i++) {
        bg = Cell::create_radial_gradient(Vector2f{sx[i], sy[i] + 2}, 3.0f, 8.0f, Color(0, 0, 0, 32), Color(0, 0, 0, 0));
        m_cell.begin_path();
        m_cell.add_rect(sx[i] - 10, sy[i] - 10 + 2, 20, 20);
        m_cell.set_fill_paint(bg);
        m_cell.fill(m_context);
    }

    m_cell.begin_path();
    for (i = 0; i < 6; i++)
        m_cell.add_circle(sx[i], sy[i], 4.0f);
    m_cell.set_fill_color(Color(0, 160, 192, 255));
    m_cell.fill(m_context);
    m_cell.begin_path();
    for (i = 0; i < 6; i++)
        m_cell.add_circle(sx[i], sy[i], 2.0f);
    m_cell.set_fill_color(Color(220, 220, 220, 255));
    m_cell.fill(m_context);

    m_cell.set_stroke_width(1.0f);
}

void Painter::drawSpinner(const Vector2f& center, const float radius, float t)
{
    float a0 = 0.0f + t * 6;
    float a1 = PI + t * 6;
    float r0 = radius;
    float r1 = radius * 0.75f;
    float ax, ay, bx, by;

    m_cell.push_state();

    m_cell.begin_path();
    m_cell.arc(center.x, center.y, r0, a0, a1, Cell::Winding::CW);
    m_cell.arc(center.x, center.y, r1, a1, a0, Cell::Winding::CCW);
    m_cell.close_path();
    ax          = center.x + cosf(a0) * (r0 + r1) * 0.5f;
    ay          = center.y + sinf(a0) * (r0 + r1) * 0.5f;
    bx          = center.x + cosf(a1) * (r0 + r1) * 0.5f;
    by          = center.y + sinf(a1) * (r0 + r1) * 0.5f;
    Paint paint = Cell::create_linear_gradient({ax, ay}, {bx, by}, Color(0, 0, 0, 0), Color(0, 0, 0, 128));
    m_cell.set_fill_paint(paint);
    m_cell.fill(m_context);

    m_cell.pop_state();
}

void Painter::drawCaps(const Vector2f& pos, const float width)
{
    Cell::LineCap caps[3] = {Cell::LineCap::BUTT, Cell::LineCap::ROUND, Cell::LineCap::SQUARE};
    float lineWidth       = 8.0f;

    m_cell.push_state();

    m_cell.begin_path();
    m_cell.add_rect(pos.x - lineWidth / 2, pos.y, width + lineWidth, width + 10);
    m_cell.set_fill_color(Color(255, 255, 255, 32));
    m_cell.fill(m_context);

    m_cell.begin_path();
    m_cell.add_rect(pos.x, pos.y, width, width + 10);
    m_cell.set_fill_color(Color(255, 255, 255, 32));
    m_cell.fill(m_context);

    m_cell.set_stroke_width(lineWidth);
    for (int i = 0; i < 3; i++) {
        m_cell.set_line_cap(caps[i]);
        m_cell.set_stroke_color(Color(0, 0, 0, 255));
        m_cell.begin_path();
        m_cell.move_to(pos.x, pos.y + i * 10 + 5);
        m_cell.line_to(pos.x + width, pos.y + i * 10 + 5);
        m_cell.stroke(m_context);
    }

    m_cell.pop_state();
}

void Painter::drawJoins(const Aabrf& rect, const float time)
{
    Cell::LineJoin joins[3] = {Cell::LineJoin::MITER, Cell::LineJoin::ROUND, Cell::LineJoin::BEVEL};
    Cell::LineCap caps[3]   = {Cell::LineCap::BUTT, Cell::LineCap::ROUND, Cell::LineCap::SQUARE};
    const float pad         = 5.0f;
    const float s           = rect.width() / 9.0f - pad * 2;

    m_cell.push_state();

    float pts[4 * 2];
    pts[0] = -s * 0.25f + cosf(time * 0.3f) * s * 0.5f;
    pts[1] = sinf(time * 0.3f) * s * 0.5f;
    pts[2] = -s * 0.25f;
    pts[3] = 0;
    pts[4] = s * 0.25f;
    pts[5] = 0;
    pts[6] = s * 0.25f + cosf(-time * 0.3f) * s * 0.5f;
    pts[7] = sinf(-time * 0.3f) * s * 0.5f;

    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 3; j++) {
            const float fx = rect.left() + s * 0.5f + (i * 3 + j) / 9.0f * rect.width() + pad;
            const float fy = rect.top() - s * 0.5f + pad;

            m_cell.set_line_cap(caps[i]);
            m_cell.set_line_join(joins[j]);

            m_cell.set_stroke_width(s * 0.3f);
            m_cell.set_stroke_color(Color(0, 0, 0, 160));
            m_cell.begin_path();
            m_cell.move_to(fx + pts[0], fy + pts[1]);
            m_cell.line_to(fx + pts[2], fy + pts[3]);
            m_cell.line_to(fx + pts[4], fy + pts[5]);
            m_cell.line_to(fx + pts[6], fy + pts[7]);
            m_cell.stroke(m_context);

            m_cell.set_line_cap(Cell::LineCap::BUTT);
            m_cell.set_line_join(Cell::LineJoin::BEVEL);

            m_cell.set_stroke_width(1.f);
            m_cell.set_stroke_color(Color(0, 192, 255, 255));
            m_cell.begin_path();
            m_cell.move_to(fx + pts[0], fy + pts[1]);
            m_cell.line_to(fx + pts[2], fy + pts[3]);
            m_cell.line_to(fx + pts[4], fy + pts[5]);
            m_cell.line_to(fx + pts[6], fy + pts[7]);
            m_cell.stroke(m_context);
        }
    }

    m_cell.pop_state();
}

void Painter::drawTexture(const Aabrf& rect)
{
    if (!test_texture) {
        test_texture = m_context.load_texture("/home/clemens/code/notf/res/textures/face.png");
    }

    Paint pattern = m_cell.create_texture_pattern(rect.top_left(), rect.extend(), test_texture, 0, 1);

    const float corner_radius = 10;

    m_cell.begin_path();
    m_cell.set_fill_paint(pattern);
    m_cell.add_rounded_rect(rect, corner_radius);
    m_cell.fill(m_context);
}

} // namespace notf
