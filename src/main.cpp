#include "core/application.hpp"
#include "core/window.hpp"
#include "core/window_layout.hpp"
#include "dynamic/controller/scroll_area.hpp"

#include "common/log.hpp"
#include "common/random.hpp"
#include "common/warnings.hpp"
#include "core/application.hpp"
#include "core/controller.hpp"
#include "core/events/char_event.hpp"
#include "core/events/focus_event.hpp"
#include "core/events/mouse_event.hpp"
#include "core/widget.hpp"
#include "core/window.hpp"
#include "core/window_layout.hpp"
#include "dynamic/layout/flex_layout.hpp"
#include "dynamic/layout/overlayout.hpp"
#include "graphics/cell/painter.hpp"
#include "graphics/graphics_context.hpp"
#include "graphics/text/font.hpp"

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
    RectWidget(GraphicsContext& context, FontPtr font, Color color)
        : Widget(), m_graphics_context(context), m_font(font), m_color(color), m_text(std::to_string(get_id().id))
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

        on_mouse_button.connect([](MouseEvent& event) -> void {
            event.set_handled();
        });

        on_focus_changed.connect([this](FocusEvent& event) -> void {
            if (event.action == FocusAction::GAINED) {
                event.set_handled();
                m_color = Color("#12d0e1");
            }
            else {
                m_color = Color("#c34200");
            }
        });

        on_char_input.connect([this](CharEvent& event) -> void {
            std::stringstream ss;
            ss << m_text << event.codepoint;
            m_text = ss.str();
        });

        UNUSED(m_graphics_context);
    }

    virtual void _paint(Painter& painter) const override
    {
        const Aabrf widget_rect = Aabrf(_get_size());
//        painter.set_scissor(widget_rect);
        painter.begin_path();
        painter.add_rect(widget_rect);
        painter.set_fill_paint(m_color);
        painter.fill();

        if (m_font) {
            painter.set_fill_paint(Color::white());
            painter.translate(widget_rect.center());
            painter.write(m_text, m_font);
        }
    }

private: // fields
    GraphicsContext& m_graphics_context;

    FontPtr m_font;

    Color m_color;

    std::string m_text;
};

/** Flex Controller
 */
class FlexController : public BaseController<FlexController> {
public: // methods
    FlexController(std::shared_ptr<Window>& window)
        : BaseController<FlexController>({}, {}), m_graphics_context(window->get_graphics_context())
    {
        std::shared_ptr<FlexLayout> flex_layout = FlexLayout::create(FlexLayout::Direction::LEFT_TO_RIGHT);
        flex_layout->set_spacing(10);
        flex_layout->set_cross_spacing(10);
        flex_layout->set_wrap(FlexLayout::Wrap::WRAP);
        _set_root_item(flex_layout);

        FontPtr font = Font::load(m_graphics_context, "/home/clemens/code/notf/res/fonts/Roboto-Regular.ttf", 12);

        for (int i = 1; i <= 50; ++i) {
            std::shared_ptr<RectWidget> rect = std::make_shared<RectWidget>(m_graphics_context, font, Color("#c34200"));
            flex_layout->add_item(rect);
        }
    }

private: // fields
    GraphicsContext& m_graphics_context;
};

class MainController : public BaseController<MainController> {
public: // methods
    MainController(std::shared_ptr<Window> window)
        : BaseController<MainController>({}, {})
    {
        std::shared_ptr<Overlayout> overlayout = Overlayout::create();
        overlayout->set_padding(Padding::all(20));
//        auto back_rect = std::make_shared<RectWidget>(window->get_graphics_context(), nullptr, Color("#333333"));
//        back_rect->set_claim({});
//        overlayout->add_item(back_rect);

        ScrollAreaPtr scroll_area     = std::make_shared<ScrollArea>();
        ControllerPtr flex_controller = std::make_shared<FlexController>(window);
        scroll_area->set_area_controller(flex_controller);
        overlayout->add_item(scroll_area);

        _set_root_item(overlayout);
    }

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
