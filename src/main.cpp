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
    RectWidget(FontPtr font, Color color)
        : Widget(), m_font(font), m_color(color), m_text(std::to_string(get_id().id))
    {
    }

    virtual void _paint(Painter& painter) const override
    {
        const Aabrf widget_rect = Aabrf(get_size());
        painter.set_scissor(widget_rect);
        painter.begin_path();
        painter.add_rect(widget_rect);
        painter.set_fill(m_color);
        painter.fill();

        if (m_font) {
            painter.set_fill(Color::white());
            painter.translate(widget_rect.center());
            painter.write(m_text, m_font);
        }
    }

private: // fields
    FontPtr m_font;

    Color m_color;

    std::string m_text;
};

class MainController : public BaseController<MainController> {
public: // methods
    MainController(std::shared_ptr<Window> window)
        : BaseController<MainController>({}, {})
    {
        std::shared_ptr<Overlayout> overlayout = Overlayout::create();
        overlayout->set_alignment(Overlayout::AlignHorizontal::RIGHT, Overlayout::AlignVertical::TOP);
//        overlayout->set_padding(Padding::all(20));
        overlayout->set_claim(Claim::fixed(400, 400));

        auto rect1 = std::make_shared<RectWidget>(nullptr, Color("#333333"));
        rect1->set_claim(Claim::fixed(100, 100));
        overlayout->add_item(rect1);

        auto rect2 = std::make_shared<RectWidget>(nullptr, Color("#ff8611"));
        rect2->set_claim(Claim::fixed(50, 200));
        overlayout->add_item(rect2);

        _set_root_item(overlayout);
    }
};

#if 1
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
#endif
