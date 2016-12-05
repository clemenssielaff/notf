#include "core/application.hpp"
#include "core/events/key_event.hpp"
#include "core/window.hpp"
#include "python/interpreter.hpp"

#include "core/foo.hpp"

using namespace notf;

int main(int argc, char* argv[])
//int notmain(int argc, char* argv[])
{
    ApplicationInfo app_info;
    app_info.argc = argc;
    app_info.argv = argv;
    Application& app = Application::initialize(app_info);

    // window
    WindowInfo window_info;
    window_info.icon = "notf.png";
    window_info.width = 800;
    window_info.height = 600;
    window_info.clear_color = Color("#262a32");
    window_info.enable_vsync = false;
    std::shared_ptr<Window> window = Window::create(window_info);

    window->on_token_key.connect(
        [window, &app](const KeyEvent&) {
            if(PythonInterpreter* python = app.get_python_interpreter()){
                python->parse_app("main.py");
            }
        },
        [](const KeyEvent& event) -> bool {
            return event.key == KEY::SPACE && event.action == KEY_ACTION::KEY_PRESS;
        });

    window->on_token_key.connect(
        [window, &app](const KeyEvent&) {
            do_the_foos();
        },
        [](const KeyEvent& event) -> bool {
            return event.key == KEY::BACKSPACE && event.action == KEY_ACTION::KEY_PRESS;
        });

    return app.exec();
}
