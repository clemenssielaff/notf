#include <glad/glad.h>
#include <glm/gtc/matrix_transform.hpp>

#include "core/application.hpp"
#include "core/components/texture_component.hpp"
#include "core/resource_manager.hpp"
#include "core/root_layout_item.hpp"
#include "core/widget.hpp"
#include "core/window.hpp"

#include "dynamic/layout/fill_layout.hpp"
#include "dynamic/render/sprite.hpp"

using namespace signal;

int main(void)
{
    Application& app = Application::get_instance();
    std::shared_ptr<Window> window;
    {
        ResourceManager& resource_manager = app.get_resource_manager();
        resource_manager.set_texture_directory("/home/clemens/code/signal-ui/res/textures");
        resource_manager.set_shader_directory("/home/clemens/code/signal-ui/res/shaders");

        WindowInfo window_info;
        window_info.width = 800;
        window_info.height = 600;
        window = Window::create(window_info);

        // setup the background
        std::shared_ptr<FillLayout> layout = FillLayout::create();
        std::shared_ptr<RootLayoutItem> root_item = window->get_root_widget();
        root_item->set_layout(layout);

        std::shared_ptr<Widget> background = Widget::create();
        layout->set_widget(background);

        std::shared_ptr<Shader> shader = resource_manager.build_shader("sprite", "sprite.vert", "sprite.frag");
        background->add_component(make_component<SpriteRenderer>(shader));

        std::shared_ptr<TextureComponent> texture_component = make_component<TextureComponent>(TextureChannels{
            {0, resource_manager.get_texture("background.jpg")}});
        background->add_component(texture_component);
    }
    return app.exec();
}
