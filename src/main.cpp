#include "core/application.hpp"
#include "core/events/key_event.hpp"
#include "core/window.hpp"
#include "python/interpreter.hpp"

using namespace notf;

#include "core/controller.hpp"
#include "core/layout_root.hpp"
#include "core/widget.hpp"
#include "graphics/painter.hpp"

class MyWidget : public Widget {

public:
    MyWidget()
        : Widget()
    {
    }

    virtual void _paint(Painter& painter) const override
    {
        painter.test();
    }
};

class MyController : public BaseController<MyController> {
public:
    MyController()
        : BaseController<MyController>({}, {})
    {
    }

    void initialize()
    {
        _set_root_item(std::make_shared<MyWidget>());
    }
};

void app_main(Window& window)
{
    auto controller = std::make_shared<MyController>();
    controller->initialize();
    window.get_layout_root()->set_controller(controller);
}

int main(int argc, char* argv[])
//int notmain(int argc, char* argv[])
{
    ApplicationInfo app_info;
    app_info.argc    = argc;
    app_info.argv    = argv;
    Application& app = Application::initialize(app_info);

    // window
    WindowInfo window_info;
    window_info.icon               = "notf.png";
    window_info.size               = {800, 600};
    window_info.clear_color        = Color("#262a32");
    window_info.is_resizeable      = true;
    std::shared_ptr<Window> window = Window::create(window_info);

    //    window->on_token_key.connect(
    //        [window, &app](const KeyEvent&) {
    //            if (PythonInterpreter* python = app.get_python_interpreter()) {
    //                python->parse_app("main.py");
    //            }
    //        },
    //        [](const KeyEvent& event) -> bool {
    //            return event.key == Key::F5 && event.action == KeyAction::PRESS;
    //        });

    app_main(*window.get());

    return app.exec();
}
