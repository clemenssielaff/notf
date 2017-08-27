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
#include "dynamic/widget/textwidget.hpp"
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

//*********************************************************************************************************************/

static FontPtr g_font;

//*********************************************************************************************************************/

/** Rect Widget.
 */
class RectWidget : public Widget {
public: // methods
    RectWidget(Color color)
        : Widget(), m_color(color)
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

        if (g_font && g_font->is_valid()) {
            painter.set_fill(Color::white());
            painter.translate(widget_rect.center());
            painter.write(get_name(), g_font);
        }
    }

private: // fields
    Color m_color;
};

class FlexController : public BaseController<FlexController> {
public: // methods
    FlexController()
        : BaseController<FlexController>({}, {})
    {
        std::shared_ptr<FlexLayout> flex_layout = FlexLayout::create();
        flex_layout->set_name("FlexLayout");
        flex_layout->set_spacing(10);
        flex_layout->set_alignment(FlexLayout::Alignment::START);
        flex_layout->set_cross_alignment(FlexLayout::Alignment::START);
        _set_root_item(flex_layout);

        for (int i = 0; i < 4; ++i) {
            std::shared_ptr<RectWidget> rect = std::make_shared<RectWidget>(Color("#c34200"));

            if (i == 1) {
                rect->set_opacity(0.5);
            }

            std::stringstream ss;
            ss << "Rect" << std::to_string(static_cast<size_t>(rect->get_id()));
            rect->set_name(ss.str());

            Claim claim = rect->get_claim();
            claim.set_fixed(100, 100 * (i + 1));
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
        set_name("MainController");

        std::shared_ptr<Overlayout> overlayout = Overlayout::create();
        overlayout->set_alignment(Overlayout::AlignHorizontal::CENTER, Overlayout::AlignVertical::CENTER);
        overlayout->set_padding(Padding::all(50));
        overlayout->set_name("Outer OverLayout");
        _set_root_item(overlayout);

        std::shared_ptr<RectWidget> back_rect = std::make_shared<RectWidget>(Color("#333333"));
        overlayout->add_item(back_rect);

        std::shared_ptr<FlexLayout> vertical_layout = FlexLayout::create(FlexLayout::Direction::TOP_TO_BOTTOM);
        vertical_layout->set_spacing(20);
        overlayout->add_item(vertical_layout);

        ScrollAreaPtr scroll_area = std::make_shared<ScrollArea>();
        scroll_area->set_name("ScrollArea");
        scroll_area->set_area_controller(std::make_shared<FlexController>());
        scroll_area->get_area_controller()->set_name("FlexController");
        vertical_layout->add_item(scroll_area);

        TextWidgetPtr text_widget = std::make_shared<TextWidget>(
            g_font, "\
Lorem ipsum dolor sit amet, consectetur adipiscing elit. Fusce eget cursus elit. Interdum et malesuada fames ac ante ipsum primis in faucibus. Suspendisse pulvinar nisi vitae lacus vestibulum ultricies. Nunc condimentum, mi ac blandit tincidunt, enim eros volutpat risus, in viverra nisi magna accumsan eros. Suspendisse lobortis sodales dignissim. Donec euismod augue et sem pulvinar, non volutpat eros accumsan. Proin dapibus luctus enim, sodales blandit ipsum laoreet in. In faucibus vitae mauris ultricies eleifend. Proin tempor massa vel ligula consequat, id elementum tellus lobortis.\
");
//Sed id tellus auctor, elementum risus vestibulum, faucibus urna. Etiam sit amet euismod velit, et finibus nulla. Nulla facilisi. Nunc quis est quis lorem dictum maximus. Maecenas ut sollicitudin orci. Aenean magna erat, ultrices pharetra dignissim vel, bibendum in elit. Nullam at ante est. Proin facilisis posuere orci, sit amet fermentum nibh auctor condimentum. Etiam quis tempor elit, tempus commodo neque. Maecenas semper augue non enim lacinia rutrum. Nam in vestibulum augue. Ut aliquet nulla a odio sodales vestibulum. Vestibulum ac dignissim ipsum, eget dictum neque. Duis efficitur mi ac efficitur porta.\
//Nam accumsan lorem vitae dapibus faucibus. Aenean pellentesque feugiat ante non porta. Nunc commodo leo in elit molestie volutpat. Donec urna ex, feugiat ac ultricies quis, sodales a massa. Morbi efficitur mi nec dui finibus faucibus. In scelerisque tristique dictum. Aenean porttitor tortor eu sapien vulputate cursus. Nam volutpat pretium neque vitae mollis. Interdum et malesuada fames ac ante ipsum primis in faucibus. Aenean nec aliquam lorem, sit amet tempus justo. Quisque ornare est est, a dictum eros scelerisque cursus. Donec rutrum quam aliquam auctor ultricies. Vestibulum vestibulum sem et mauris auctor, nec aliquet purus molestie. Integer consectetur scelerisque ligula nec venenatis.\
//Donec rutrum condimentum orci eget cursus. Mauris in sapien vitae felis sollicitudin fermentum eu quis lorem. Nullam rutrum tristique nisi. Sed sed arcu quis odio vulputate varius. Aenean molestie nunc et nulla volutpat tempor. Aliquam fringilla erat a lacus sollicitudin, porttitor accumsan turpis elementum. Nulla sit amet orci quis nibh auctor porta. Curabitur nec posuere nibh. Praesent at vestibulum nisi, sit amet viverra felis. Suspendisse aliquam, massa ac congue vulputate, turpis lectus molestie lectus, sed molestie nisi risus nec nibh. Praesent ex ex, tempus at metus eu, vehicula venenatis odio. Interdum et malesuada fames ac ante ipsum primis in faucibus. Vivamus dignissim dictum porta. Proin sit amet nibh molestie, mollis quam nec, dapibus turpis.\
//Donec vehicula dapibus leo, non tempus nunc maximus et. Sed non ex est. Pellentesque dictum a felis quis dignissim. Pellentesque ultrices velit ipsum, eget dictum nisi tincidunt sed. Maecenas eget sollicitudin dui, id condimentum ex. Pellentesque velit dui, euismod et turpis sed, ornare tincidunt tortor. Maecenas eu libero consequat, tincidunt augue fermentum, suscipit tellus. Sed at magna neque.\

        text_widget->set_wrapping(true);
        vertical_layout->add_item(text_widget);
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

    g_font = Font::load(window->get_graphics_context(),
                        "/home/clemens/code/BUILD/notf-debug/res/fonts/Roboto-Regular.ttf",
                        11);

    window->get_layout()->set_controller(std::make_shared<MainController>());
    int result = app.exec();

    g_font.reset();

    return result;
}
#endif
