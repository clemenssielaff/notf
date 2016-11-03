#include "common/log.hpp"
#include "core/application.hpp"
#include "core/components/texture_component.hpp"
#include "core/events/key_event.hpp"
#include "core/layout_root.hpp"
#include "core/resource_manager.hpp"
#include "core/widget.hpp"
#include "core/window.hpp"
#include "python/interpreter.hpp"

#include "dynamic/layout/stack_layout.hpp"
#include "dynamic/render/sprite.hpp"

using namespace notf;

int main(int argc, char* argv[])
{
    Application& app = Application::initialize(argc, argv);

    // resource manager
    ResourceManager& resource_manager = app.get_resource_manager();
    resource_manager.set_texture_directory("/home/clemens/code/notf/res/textures");
    resource_manager.set_shader_directory("/home/clemens/code/notf/res/shaders");

    // window
    WindowInfo window_info;
    window_info.icon = "notf.png";
    window_info.width = 800;
    window_info.height = 600;
    window_info.opengl_version_major = 3;
    window_info.opengl_version_minor = 3;
    window_info.opengl_remove_deprecated = true;
    window_info.opengl_profile = WindowInfo::PROFILE::CORE;
    window_info.enable_vsync = false;
    std::shared_ptr<Window> window = Window::create(window_info);

    window->on_token_key.connect(
        [window, &app](const KeyEvent&) {
            PythonInterpreter& python = app.get_python_interpreter();
            python.parse_app();

        },
        [](const KeyEvent& event) -> bool {
            return event.key == KEY::SPACE && event.action == KEY_ACTION::KEY_PRESS;
        });

    return app.exec();
}
