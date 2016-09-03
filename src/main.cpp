#include <glad/glad.h>
#include <glm/gtc/matrix_transform.hpp>

#include "core/application.hpp"
#include "core/components/texture_component.hpp"
#include "core/widget.hpp"
#include "core/window.hpp"
#include "dynamic/render/sprite_component.hpp"

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
    //    window_info.is_resizeable = false;
    Window window(window_info);

    {
        std::shared_ptr<Widget> widget = Widget::make_widget();
        widget->set_parent(window.get_root_widget());

        std::shared_ptr<Shader> shader = resource_manager.build_shader("sprite", "sprite.vert", "sprite.frag");
        widget->add_component(make_component<SpriteComponent>(shader));

        std::shared_ptr<TextureComponent> texture_component = make_component<TextureComponent>(TextureChannels{
            {0, resource_manager.get_texture("background.jpg")}});
        widget->add_component(texture_component);
    }

    return app.exec();
}
