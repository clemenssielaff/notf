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

class FlexController : public BaseController<FlexController> {
public: // methods
    FlexController()
        : BaseController<FlexController>({}, {})
    {
//        std::shared_ptr<Overlayout> overlayout = Overlayout::create();
//        _set_root_item(overlayout);

//        std::shared_ptr<RectWidget> back_rect = std::make_shared<RectWidget>(nullptr, Color("#666666"));
//        Claim claim;
//        claim.set_active(false);
//        back_rect->set_claim(claim);
//        overlayout->add_item(back_rect);

        std::shared_ptr<FlexLayout> flex_layout = FlexLayout::create();
        flex_layout->set_spacing(10);
        flex_layout->set_alignment(FlexLayout::Alignment::END);
//        back_rect->connect_signal(flex_layout->on_size_changed, [back_rect](const Size2f& size) -> void {
//            Claim claim = Claim::fixed(size);
//            claim.set_active(false);
//            back_rect->set_claim(std::move(claim));
//        });
//        overlayout->add_item(flex_layout);
        _set_root_item(flex_layout);

        for (int i = 0; i < 2; ++i) {
            std::shared_ptr<RectWidget> rect = std::make_shared<RectWidget>(nullptr, Color("#c34200"));
            Claim claim                      = rect->get_claim();
            claim.set_fixed(100, 100);
            rect->set_claim(claim);
            flex_layout->add_item(rect);
        }
    }
};

class MainController : public BaseController<MainController> {
public: // methods
    MainController()
        : BaseController<MainController>({}, {})
    {
        std::shared_ptr<Overlayout> overlayout = Overlayout::create();
        overlayout->set_padding(Padding::all(20));
        overlayout->set_alignment(Overlayout::AlignHorizontal::CENTER, Overlayout::AlignVertical::CENTER);
        log_info << "Overlayout in question has ID:" << overlayout->get_id();

        std::shared_ptr<RectWidget> back_rect = std::make_shared<RectWidget>(nullptr, Color("#333333"));
        overlayout->add_item(back_rect);

        ControllerPtr flex_controller = std::make_shared<FlexController>();
        overlayout->add_item(flex_controller);

        _set_root_item(overlayout);
    }
};

#if 1
int main(int argc, char* argv[])
{
    ApplicationInfo app_info;
    app_info.argc    = argc;
    app_info.argv    = argv;
    Application& app = Application::initialize(app_info);

    // window
    WindowInfo window_info;
    window_info.icon          = "notf.png";
    window_info.size          = {800, 600};
    window_info.clear_color   = Color("#262a32");
    window_info.is_resizeable = true;
    auto window               = Window::create(window_info);

    window->get_layout()->set_controller(std::make_shared<MainController>());
    return app.exec();
}
#endif
