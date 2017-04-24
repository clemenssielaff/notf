#include "core/application.hpp"
#include "core/window.hpp"
#include "core/window_layout.hpp"
#include "dynamic/controller/scroll_area.hpp"

#include "common/random.hpp"
#include "core/application.hpp"
#include "core/controller.hpp"
#include "core/widget.hpp"
#include "core/window.hpp"
#include "core/window_layout.hpp"
#include "dynamic/layout/overlayout.hpp"
#include "dynamic/layout/stack_layout.hpp"
#include "graphics/cell/painter.hpp"
#include "graphics/graphics_context.hpp"
#include "graphics/text/font.hpp"
#include "utils/unused.hpp"

namespace notf {
namespace shorthand {
using WindowPtr     = std::shared_ptr<Window>;
using FontPtr       = std::shared_ptr<Font>;
using WidgetPtr     = std::shared_ptr<Widget>;
using ControllerPtr = std::shared_ptr<Controller>;
}
}

using namespace notf;
using namespace notf::shorthand;

/** Rect Widget.
 */
class RectWidget : public Widget {
public: // methods
    RectWidget(GraphicsContext& context, int number, FontPtr font, Color color)
        : Widget(), m_graphics_context(context), m_font(font), m_number(number), m_color(color)
    {
        const float min_min = 20;
        const float max_min = 100;
        const float max_max = 200;

        Claim::Stretch horizontal, vertical;
        horizontal.set_min(random_number(min_min, max_min));
        horizontal.set_max(random_number(horizontal.get_min(), max_max));
        horizontal.set_preferred(random_number(horizontal.get_min(), horizontal.get_max()));
        vertical.set_min(random_number(min_min, max_min));
        vertical.set_max(random_number(vertical.get_min(), max_max));
        vertical.set_preferred(random_number(vertical.get_min(), vertical.get_max()));
        set_claim({horizontal, vertical});

        UNUSED(m_graphics_context);
    }

    virtual void _paint(Painter& painter) const override
    {
        const Aabrf widget_rect = Aabrf(get_size());
        painter.begin_path();
        painter.add_rect(widget_rect);
        painter.set_fill_paint(m_color);
        painter.fill();

        if (m_font) {
            painter.set_fill_paint(Color::white());
            painter.translate(widget_rect.center());
            painter.write(std::to_string(m_number), m_font);
        }
    }

private: // fields
    GraphicsContext& m_graphics_context;

    FontPtr m_font;

    int m_number;

    Color m_color;
};

/** Stack Controller
 */
class StackController : public BaseController<StackController> {
public: // methods
    StackController(std::shared_ptr<Window>& window)
        : BaseController<StackController>({}, {}), m_graphics_context(window->get_graphics_context()) {}

    virtual void _initialize() override
    {
        StackLayoutPtr stack_layout = StackLayout::create(StackLayout::Direction::LEFT_TO_RIGHT);
        stack_layout->set_spacing(10);
        stack_layout->set_cross_spacing(10);
        stack_layout->set_wrap(StackLayout::Wrap::WRAP);
        _set_root_item(stack_layout);

        FontPtr font = Font::load(m_graphics_context, "/home/clemens/code/notf/res/fonts/Roboto-Regular.ttf", 12);

        for (int i = 1; i <= 20; ++i) {
            std::shared_ptr<RectWidget> rect = std::make_shared<RectWidget>(m_graphics_context, i, font, Color("#c34200"));
            stack_layout->add_item(rect);
            rect->set_scissor(stack_layout);
        }
    }

private: // fields
    GraphicsContext& m_graphics_context;
};

class MainController : public BaseController<MainController> {
public: // methods
    MainController(std::shared_ptr<Window> window)
        : BaseController<MainController>({}, {}), m_window(window) {}

    virtual void _initialize() override
    {
        OverlayoutPtr overlayout = Overlayout::create();
        overlayout->set_padding(Padding::all(20));
        auto back_rect = std::make_shared<RectWidget>(m_window->get_graphics_context(), 0, nullptr, Color("#333333"));
        back_rect->set_claim({});
        overlayout->add_item(back_rect);

        ScrollAreaPtr scroll_area = std::make_shared<ScrollArea>();
        scroll_area->initialize(); // TODO: this Controller::initialize stuff is really brittle, maybe combine TODO with Controller::Factory?
        ControllerPtr stack_controller = std::make_shared<StackController>(m_window);
        scroll_area->set_area_controller(stack_controller);

        overlayout->add_item(scroll_area);
        _set_root_item(overlayout);
    }

private: // fields
    std::shared_ptr<Window> m_window;
};

int main(int argc, char* argv[])
{
    ApplicationInfo app_info;
    app_info.argc         = argc;
    app_info.argv         = argv;
    app_info.enable_vsync = false;
    Application& app      = Application::initialize(app_info);

    // window
    WindowInfo window_info;
    window_info.icon          = "notf.png";
    window_info.size          = {800, 600};
    window_info.clear_color   = Color("#262a32");
    window_info.is_resizeable = true;
    auto window               = Window::create(window_info);

    window->get_layout()->set_controller(std::make_shared<MainController>(window));

    return app.exec();
}
