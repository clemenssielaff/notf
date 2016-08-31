#include "core/application.hpp"
#include "graphics/sprite_component.hpp"
#include "core/widget.hpp"
#include "core/window.hpp"
using namespace signal;

int main(void)
{
    Application& app = Application::get_instance();
    ResourceManager& resource_manager = app.get_resource_manager();
    resource_manager.set_texture_directory("/home/clemens/code/signal-ui/res/textures");
    resource_manager.set_shader_directory("/home/clemens/code/signal-ui/res/shaders");

    WindowInfo window_info;
    window_info.enable_vsync = false; // to correctly time each frame
    window_info.width = 800;
    window_info.height = 600;
    window_info.is_resizeable = false;
    Window window(window_info);

    std::shared_ptr<Widget> widget = Widget::make_widget();
    widget->set_parent(window.get_root_widget());

    auto shader = resource_manager.build_shader("sprite", "sprite.vert", "sprite.frag");
    auto texture = resource_manager.get_texture("background.jpg");
    widget->set_component(make_component<SpriteComponent>(shader, texture));

    return app.exec();
}
