#include "core/application.hpp"
#include "core/events/key_event.hpp"
#include "core/window.hpp"
#include "python/interpreter.hpp"

using namespace notf;

#include "core/controller.hpp"
#include "core/render_manager.hpp"
#include "core/widget.hpp"
#include "core/window_layout.hpp"
#include "graphics/cell/painter.hpp"
#include "graphics/graphics_context.hpp"
#include "graphics/text/font_manager.hpp"
#include "graphics/texture2.hpp"

GraphicsContext* g_graphics_context = nullptr;
FontManager* g_font_manager         = nullptr;
std::shared_ptr<Font> g_font;

class MyWidget : public Widget {

public:
    MyWidget(GraphicsContext& context)
        : Widget()
        , test_texture(Texture2::load_image(context, "/home/clemens/code/notf/res/textures/face.png"))
    {
    }

    virtual void _paint(Painter& painter) const override
    {
        const Size2f widget_size = get_size();
        const Aabrf base(widget_size);
        const float margin = 20;
        const float time   = static_cast<float>(painter.get_time().in_seconds());

        //        painter.begin_path();
        //        painter.add_circle(base.center(), 80);
        //        painter.set_fill_paint(Color(255, 0, 0));
        //        painter.fill();

        drawGraph(painter, base, time);

        drawColorwheel(painter, base.shrunken(margin), time);

        drawCheckBox(painter, Aabrf{10, 100, 20, 20});

        drawButton(painter, Aabrf{10, 130, 150, 30});

        drawSlider(painter, Aabrf{10, 170, 150, 30}, 0.4f);

        drawCaps(painter, Vector2f{10, 200}, 30);

        drawEyes(painter, Aabrf{600, 20, 80, 60}, painter.get_mouse_pos(), time);

        drawSpinner(painter, base.center(), 100, time);

        drawJoins(painter, Aabrf{120, widget_size.height - 50, 600, 50}, time);

        drawTexture(painter, Aabrf{400, 100, 200, 200});

        painter.translate(192, 200);
        painter.write("This is a test text that I would like to see printed on screen please", g_font);
    }

    void drawSlider(Painter& painter, const Aabrf& rect, float pos) const
    {
        float kr = (int)(rect.height() * 0.25f);

        painter.push_state();

        // Slot
        Paint bg = Paint::create_box_gradient(Vector2f{rect.left(), rect.y() - 2 + 1},
                                              Size2f{rect.width(), 4}, 2, 2, Color(0, 0, 0, 32), Color(0, 0, 0, 128));
        painter.begin_path();
        painter.add_rounded_rect(rect.left(), rect.y() - 2, rect.width(), 4, 2);
        painter.set_fill_paint(bg);
        painter.fill();

        // Knob Shadow
        bg = Paint::create_radial_gradient(Vector2f{rect.left() + (int)(pos * rect.width()), rect.y() + 1},
                                           kr - 3, kr + 3, Color(0, 0, 0, 64), Color(0, 0, 0, 0));
        painter.begin_path();
        painter.add_rect(rect.left() + (int)(pos * rect.width()) - kr - 5, rect.y() - kr - 5, kr * 2 + 5 + 5, kr * 2 + 5 + 5 + 3);
        painter.add_circle(rect.left() + (int)(pos * rect.width()), rect.y(), kr);
        painter.set_winding(Painter::Winding::HOLE);
        painter.set_fill_paint(bg);
        painter.fill();

        // Knob
        Paint knob = Paint::create_linear_gradient(Vector2f{rect.left(), rect.y() - kr},
                                                   Vector2f{rect.left(), rect.y() + kr},
                                                   Color(255, 255, 255, 16), Color(0, 0, 0, 16));
        painter.begin_path();
        painter.add_circle(rect.left() + (int)(pos * rect.width()), rect.y(), kr - 1);
        painter.set_fill_color(Color(40, 43, 48, 255));
        painter.fill();
        painter.set_fill_paint(knob);
        painter.fill();

        painter.begin_path();
        painter.add_circle(rect.left() + (int)(pos * rect.width()), rect.y(), kr - 0.5f);
        painter.set_stroke_color(Color(0, 0, 0, 92));
        painter.stroke();

        painter.pop_state();
    }

    void drawButton(Painter& painter, const Aabrf& rect) const
    {
        const float corner_radius = 4;
        Paint gradient            = Paint::create_linear_gradient(rect.top_left(), rect.bottom_left(),
                                                       Color(0, 0, 0, 32), Color(255, 255, 255, 32));

        painter.begin_path();
        painter.set_fill_color(Color(0, 96, 128));
        painter.add_rounded_rect(rect, corner_radius - 1);
        painter.stroke();
        painter.fill();

        painter.begin_path();
        painter.add_rounded_rect(rect, corner_radius - 1);
        painter.set_fill_paint(gradient);
        painter.fill();
    }

    void drawCheckBox(Painter& painter, const Aabrf& rect) const
    {
        Paint gradient = Paint::create_box_gradient(Vector2f{rect.left() + 1, rect.y() - 8},
                                                    Size2f{18, 18}, 3, 3, Color(0, 0, 0, 32), Color(0, 0, 0, 92));
        painter.quad_to(100.f, 100.f, 200.f, 200.f);
        painter.begin_path();
        painter.add_rounded_rect(rect.left() + 1, rect.y() - 9, 18, 18, 3);
        painter.set_fill_paint(gradient);
        painter.fill();
    }

    void drawColorwheel(Painter& painter, const Aabrf& rect, float t) const
    {
        float hue = sin(t * 0.12f);

        painter.push_state();

        const float outer_radius = rect.shorter_size() * 0.5f - 5.0f;
        const float inner_radius = outer_radius - 20.0f;
        if (inner_radius <= 0) {
            return;
        }
        const float aeps = 0.5f / outer_radius; // half a pixel arc length in radians (2pi cancels out).

        // hue circle
        for (int i = 0; i < 6; i++) {
            float a0 = static_cast<float>(i) / 6.0f * TWO_PI - aeps;
            float a1 = static_cast<float>(i + 1) / 6.0f * TWO_PI + aeps;
            painter.begin_path();
            painter.arc(rect.x(), rect.y(), inner_radius, a0, a1, Painter::Winding::CLOCKWISE);
            painter.arc(rect.x(), rect.y(), outer_radius, a1, a0, Painter::Winding::COUNTERCLOCKWISE);
            painter.close_path();

            const Vector2f start_pos{rect.x() + cosf(a0) * (inner_radius + outer_radius) * 0.5f, rect.y() + sinf(a0) * (inner_radius + outer_radius) * 0.5f};
            const Vector2f end_pos{rect.x() + cosf(a1) * (inner_radius + outer_radius) * 0.5f, rect.y() + sinf(a1) * (inner_radius + outer_radius) * 0.5f};
            const Color start_color = Color::from_hsl(a0, 1.0f, 0.55f, 1);
            const Color end_color   = Color::from_hsl(a1, 1.0f, 0.55f, 1);
            painter.set_fill_paint(Paint::create_linear_gradient(start_pos, end_pos, start_color, end_color));
            painter.fill();
        }

        // border around hue circle
        painter.begin_path();
        painter.add_circle(rect.x(), rect.y(), inner_radius - 0.5f);
        painter.add_circle(rect.x(), rect.y(), outer_radius + 0.5f);
        painter.set_stroke_color(Color(0, 0, 0, 64));
        painter.set_stroke_width(1.0f);
        painter.stroke();

        // Selector
        painter.push_state();
        painter.translate(rect.center());
        painter.rotate(hue * TWO_PI);

        // marker on circle
        painter.begin_path();
        painter.add_rect(inner_radius - 2, -3, outer_radius - inner_radius + 4, 6);
        painter.set_stroke_width(2.0f);
        painter.set_stroke_color(Color(255, 255, 255, 192));
        painter.stroke();

        painter.begin_path();
        painter.add_rect(inner_radius - 2 - 10, -4 - 10, outer_radius - inner_radius + 4 + 20, 8 + 20);
        painter.add_rect(inner_radius - 2, -4, outer_radius - inner_radius + 4, 8);
        painter.set_winding(Painter::Winding::HOLE);
        const Vector2f center{inner_radius - 3, -5};
        const Size2f extend{outer_radius - inner_radius + 6, 10};
        painter.set_fill_paint(Paint::create_box_gradient(center, extend, 2, 4, Color(0, 0, 0, .5), Color(0, 0, 0, 0)));
        painter.fill();

        const float r = inner_radius - 6;

        // center triangle
        {
            const float ax = cosf(120.0f / 180.0f * PI) * r;
            const float ay = sinf(120.0f / 180.0f * PI) * r;
            const float bx = cosf(-120.0f / 180.0f * PI) * r;
            const float by = sinf(-120.0f / 180.0f * PI) * r;
            painter.begin_path();
            painter.move_to(r, 0);
            painter.line_to(ax, ay);
            painter.line_to(bx, by);
            painter.close_path();

            Vector2f start_pos{r, 0};
            Vector2f end_pos{ax, ay};
            Color start_color = Color::from_hsl(hue * TWO_PI, 1.0f, 0.5f, 1);
            Color end_color   = Color(1, 1, 1, 1.);
            painter.set_fill_paint(Paint::create_linear_gradient(start_pos, end_pos, start_color, end_color));
            painter.fill();

            start_pos   = Vector2f{(r + ax) * 0.5f, (0 + ay) * 0.5f};
            end_pos     = Vector2f{bx, by};
            start_color = Color(0, 0, 0, 0.);
            end_color   = Color(0, 0, 0, 1.);
            painter.set_fill_paint(Paint::create_linear_gradient(start_pos, end_pos, start_color, end_color));
            painter.fill();
            painter.set_stroke_color(Color(0, 0, 0, 64));
            painter.stroke();
        }

        // Select circle on triangle
        const float ax = cosf(120.0f / 180.0f * PI) * r * 0.3f;
        const float ay = sinf(120.0f / 180.0f * PI) * r * 0.4f;
        painter.set_stroke_width(2);
        painter.begin_path();
        painter.add_circle(ax, ay, 5);
        painter.set_stroke_color(Color(1, 1, 1, 192));
        painter.stroke();

        painter.begin_path();
        painter.add_rect(ax - 20, ay - 20, 40, 40);
        painter.add_circle(ax, ay, 7);
        painter.set_winding(Painter::Winding::HOLE);
        painter.set_fill_paint(Paint::create_radial_gradient(Vector2f{ax, ay}, 7, 9, Color(0, 0, 0, 64), Color(0, 0, 0, 0)));
        painter.fill();

        painter.pop_state();

        painter.pop_state();
    }

    void drawEyes(Painter& painter, const Aabrf& rect, const Vector2f& target, float t) const
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

        Paint bg = Paint::create_linear_gradient(Vector2f{rect.left(), rect.top() + rect.height() * 0.5f},
                                                 Vector2f{rect.left() + rect.width() * 0.1f, rect.top() + rect.height()},
                                                 Color(0, 0, 0, 32), Color(0, 0, 0, 16));
        painter.begin_path();
        painter.add_ellipse(lx + 3.0f, ly + 16.0f, ex, ey);
        painter.add_ellipse(rx + 3.0f, ry + 16.0f, ex, ey);
        painter.set_fill_paint(bg);
        painter.fill();

        bg = Paint::create_linear_gradient({rect.left(), rect.top() + rect.height() * 0.25f},
                                           {rect.left() + rect.width() * 0.1f, rect.top() + rect.height()}, Color(220, 220, 220, 255), Color(128, 128, 128, 255));
        painter.begin_path();
        painter.add_ellipse(lx, ly, ex, ey);
        painter.add_ellipse(rx, ry, ex, ey);
        painter.set_fill_paint(bg);
        painter.fill();

        dx = (target.x - rx) / (ex * 10);
        dy = (target.y - ry) / (ey * 10);
        d  = sqrtf(dx * dx + dy * dy);
        if (d > 1.0f) {
            dx /= d;
            dy /= d;
        }
        dx *= ex * 0.4f;
        dy *= ey * 0.5f;
        painter.begin_path();
        painter.add_ellipse(lx + dx, ly + dy + ey * 0.25f * (1 - blink), br, br * blink);
        painter.set_fill_color(Color(32, 32, 32, 255));
        painter.fill();

        dx = (target.x - rx) / (ex * 10);
        dy = (target.y - ry) / (ey * 10);
        d  = sqrtf(dx * dx + dy * dy);
        if (d > 1.0f) {
            dx /= d;
            dy /= d;
        }
        dx *= ex * 0.4f;
        dy *= ey * 0.5f;
        painter.begin_path();
        painter.add_ellipse(rx + dx, ry + dy + ey * 0.25f * (1 - blink), br, br * blink);
        painter.set_fill_color(Color(32, 32, 32, 255));
        painter.fill();

        Paint gloss = Paint::create_radial_gradient(Vector2f{lx - ex * 0.25f, ly - ey * 0.5f}, ex * 0.1f, ex * 0.75f, Color(255, 255, 255, 128), Color(255, 255, 255, 0));
        painter.begin_path();
        painter.add_ellipse(lx, ly, ex, ey);
        painter.set_fill_paint(gloss);
        painter.fill();

        gloss = Paint::create_radial_gradient(Vector2f{rx - ex * 0.25f, ry - ey * 0.5f}, ex * 0.1f, ex * 0.75f, Color(255, 255, 255, 128), Color(255, 255, 255, 0));
        painter.begin_path();
        painter.add_ellipse(rx, ry, ex, ey);
        painter.set_fill_paint(gloss);
        painter.fill();
    }

    void drawGraph(Painter& painter, const Aabrf& rect, float t) const
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
        Paint bg = Paint::create_linear_gradient(rect.top_left(), rect.bottom_left(), Color(0, 160, 192, 0), Color(0, 160, 192, 64));
        painter.begin_path();
        painter.move_to(sx[0], sy[0]);
        for (i = 1; i < 6; i++)
            painter.bezier_to(sx[i - 1] + dx * 0.5f, sy[i - 1], sx[i] - dx * 0.5f, sy[i], sx[i], sy[i]);
        painter.line_to(rect.left() + rect.width(), rect.top() + rect.height());
        painter.line_to(rect.left(), rect.top() + rect.height());
        painter.set_fill_paint(bg);
        painter.fill();

        // Graph line
        painter.begin_path();
        painter.move_to(sx[0], sy[0] + 2);
        for (i = 1; i < 6; i++)
            painter.bezier_to(sx[i - 1] + dx * 0.5f, sy[i - 1] + 2, sx[i] - dx * 0.5f, sy[i] + 2, sx[i], sy[i] + 2);
        painter.set_stroke_color(Color(0, 0, 0, 32));
        painter.set_stroke_width(3.0f);
        painter.stroke();

        painter.begin_path();
        painter.move_to(sx[0], sy[0]);
        for (i = 1; i < 6; i++)
            painter.bezier_to(sx[i - 1] + dx * 0.5f, sy[i - 1], sx[i] - dx * 0.5f, sy[i], sx[i], sy[i]);
        painter.set_stroke_color(Color(0, 160, 192, 255));
        painter.set_stroke_width(3.0f);
        painter.stroke();

        // Graph sample pos
        for (i = 0; i < 6; i++) {
            bg = Paint::create_radial_gradient(Vector2f{sx[i], sy[i] + 2}, 3.0f, 8.0f, Color(0, 0, 0, 32), Color(0, 0, 0, 0));
            painter.begin_path();
            painter.add_rect(sx[i] - 10, sy[i] - 10 + 2, 20, 20);
            painter.set_fill_paint(bg);
            painter.fill();
        }

        painter.begin_path();
        for (i = 0; i < 6; i++)
            painter.add_circle(sx[i], sy[i], 4.0f);
        painter.set_fill_color(Color(0, 160, 192, 255));
        painter.fill();
        painter.begin_path();
        for (i = 0; i < 6; i++)
            painter.add_circle(sx[i], sy[i], 2.0f);
        painter.set_fill_color(Color(220, 220, 220, 255));
        painter.fill();

        painter.set_stroke_width(1.0f);
    }

    void drawSpinner(Painter& painter, const Vector2f& center, const float radius, float t) const
    {
        float a0 = 0.0f + t * 6;
        float a1 = PI + t * 6;
        float r0 = radius;
        float r1 = radius * 0.75f;
        float ax, ay, bx, by;

        painter.push_state();

        painter.begin_path();
        painter.arc(center.x, center.y, r0, a0, a1, Painter::Winding::CW);
        painter.arc(center.x, center.y, r1, a1, a0, Painter::Winding::CCW);
        painter.close_path();
        ax          = center.x + cosf(a0) * (r0 + r1) * 0.5f;
        ay          = center.y + sinf(a0) * (r0 + r1) * 0.5f;
        bx          = center.x + cosf(a1) * (r0 + r1) * 0.5f;
        by          = center.y + sinf(a1) * (r0 + r1) * 0.5f;
        Paint paint = Paint::create_linear_gradient({ax, ay}, {bx, by}, Color(0, 0, 0, 0), Color(0, 0, 0, 128));
        painter.set_fill_paint(paint);
        painter.fill();

        painter.pop_state();
    }

    void drawCaps(Painter& painter, const Vector2f& pos, const float width) const
    {
        Painter::LineCap caps[3] = {Painter::LineCap::BUTT, Painter::LineCap::ROUND, Painter::LineCap::SQUARE};
        float lineWidth          = 8.0f;

        painter.push_state();

        painter.begin_path();
        painter.add_rect(pos.x - lineWidth / 2, pos.y, width + lineWidth, width + 10);
        painter.set_fill_color(Color(255, 255, 255, 32));
        painter.fill();

        painter.begin_path();
        painter.add_rect(pos.x, pos.y, width, width + 10);
        painter.set_fill_color(Color(255, 255, 255, 32));
        painter.fill();

        painter.set_stroke_width(lineWidth);
        for (int i = 0; i < 3; i++) {
            painter.set_line_cap(caps[i]);
            painter.set_stroke_color(Color(0, 0, 0, 255));
            painter.begin_path();
            painter.move_to(pos.x, pos.y + i * 10 + 5);
            painter.line_to(pos.x + width, pos.y + i * 10 + 5);
            painter.stroke();
        }

        painter.pop_state();
    }

    void drawJoins(Painter& painter, const Aabrf& rect, const float time) const
    {
        Painter::LineJoin joins[3] = {Painter::LineJoin::MITER, Painter::LineJoin::ROUND, Painter::LineJoin::BEVEL};
        Painter::LineCap caps[3]   = {Painter::LineCap::BUTT, Painter::LineCap::ROUND, Painter::LineCap::SQUARE};
        const float pad            = 5.0f;
        const float s              = rect.width() / 9.0f - pad * 2;

        painter.push_state();

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

                painter.set_line_cap(caps[i]);
                painter.set_line_join(joins[j]);

                painter.set_stroke_width(s * 0.3f);
                painter.set_stroke_color(Color(0, 0, 0, 160));
                painter.begin_path();
                painter.move_to(fx + pts[0], fy + pts[1]);
                painter.line_to(fx + pts[2], fy + pts[3]);
                painter.line_to(fx + pts[4], fy + pts[5]);
                painter.line_to(fx + pts[6], fy + pts[7]);
                painter.stroke();

                painter.set_line_cap(Painter::LineCap::BUTT);
                painter.set_line_join(Painter::LineJoin::BEVEL);

                painter.set_stroke_width(1.f);
                painter.set_stroke_color(Color(0, 192, 255, 255));
                painter.begin_path();
                painter.move_to(fx + pts[0], fy + pts[1]);
                painter.line_to(fx + pts[2], fy + pts[3]);
                painter.line_to(fx + pts[4], fy + pts[5]);
                painter.line_to(fx + pts[6], fy + pts[7]);
                painter.stroke();
            }
        }

        painter.pop_state();
    }

    void drawTexture(Painter& painter, const Aabrf& rect) const
    {
        Paint pattern = Paint::create_texture_pattern(rect.top_left(), rect.extend(), test_texture, 0, 1);

        const float corner_radius = 10;

        painter.begin_path();
        painter.set_fill_paint(pattern);
        painter.add_rounded_rect(rect, corner_radius);
        painter.fill();
    }

    std::shared_ptr<Texture2> test_texture;
};

class MyController : public BaseController<MyController> {
public:
    MyController()
        : BaseController<MyController>({}, {})
    {
    }

    virtual void _initialize() override
    {
        _set_root_item(std::make_shared<MyWidget>(*g_graphics_context));
    }
};

void app_main(Window& window)
{
    auto controller = std::make_shared<MyController>();
    window.get_layout()->set_controller(controller);
}

int main(int argc, char* argv[])
{
    ApplicationInfo app_info;
    app_info.argc         = argc;
    app_info.argv         = argv;
    app_info.enable_vsync = false;
    Application& app      = Application::initialize(app_info);

    // window
    WindowInfo window_info;
    window_info.icon               = "notf.png";
    window_info.size               = {800, 600};
    window_info.clear_color        = Color("#262a32");
    window_info.is_resizeable      = true;
    std::shared_ptr<Window> window = Window::create(window_info);

    g_graphics_context = &(window->get_graphics_context());
    g_font_manager     = &(g_graphics_context->get_font_manager());

    g_font = Font::load(*g_graphics_context, "/home/clemens/code/notf/res/fonts/Roboto-Regular.ttf", 10);

    app_main(*window.get());

    return app.exec();
}
