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
    RectWidget(GraphicsContext& context, int number, FontPtr font)
        : Widget(), m_graphics_context(context), m_font(font), m_number(number)
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
        painter.set_fill_paint(Color("#c34200"));
        painter.fill();

        painter.set_fill_paint(Color::white());
        painter.translate(widget_rect.center());
        painter.write(std::to_string(m_number), m_font);
    }

private: // fields
    GraphicsContext& m_graphics_context;

    FontPtr m_font;

    int m_number;
};

/** Stack Controller
 */
class StackController : public BaseController<StackController> {
public: // methods
    StackController(std::shared_ptr<Window>& window)
        : BaseController<StackController>({}, {}), m_graphics_context(window->get_graphics_context()), m_layout() {}

    virtual void _initialize() override
    {
        m_layout = StackLayout::create(StackLayout::Direction::LEFT_TO_RIGHT);
        m_layout->set_padding(Padding::all(20));
        m_layout->set_spacing(10);
        m_layout->set_cross_spacing(10);
        m_layout->set_wrap(StackLayout::Wrap::WRAP);
        _set_root_item(m_layout);

        FontPtr font = Font::load(m_graphics_context, "/home/clemens/code/notf/res/fonts/Roboto-Regular.ttf", 12);

        for (int i = 1; i <= 20; ++i) {
            m_layout->add_item(std::make_shared<RectWidget>(m_graphics_context, i, font));
        }
    }

private: // fields
    GraphicsContext& m_graphics_context;

    std::shared_ptr<StackLayout> m_layout;
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

    std::shared_ptr<ScrollArea> scroll_area = std::make_shared<ScrollArea>();
    ControllerPtr controller = std::make_shared<StackController>(window);
    window->get_layout()->set_controller(scroll_area);
    scroll_area->set_area_controller(controller); // TODO: this Controller::initialize stuff is really brittle, maybe combine TODO with Controller::Factory?

    return app.exec();
}
