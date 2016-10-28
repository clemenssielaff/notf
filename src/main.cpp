#include <glad/glad.h>
#include <glm/gtc/matrix_transform.hpp>

#include "core/application.hpp"
#include "core/components/texture_component.hpp"
#include "core/layout_root.hpp"
#include "core/resource_manager.hpp"
#include "core/widget.hpp"
#include "core/window.hpp"

#include "dynamic/render/sprite.hpp"
#include "dynamic/layout/stack_layout.hpp"

using namespace notf;

int main(void)
//int notmain(void)
{
    Application& app = Application::get_instance();
    std::shared_ptr<Window> window;
    {
        // resource manager
        ResourceManager& resource_manager = app.get_resource_manager();
        resource_manager.set_texture_directory("/home/clemens/code/notf/res/textures");
        resource_manager.set_shader_directory("/home/clemens/code/notf/res/shaders");

        // window
        WindowInfo window_info;
        window_info.width = 800;
        window_info.height = 600;
        window_info.opengl_version_major = 3;
        window_info.opengl_version_minor = 3;
        window_info.opengl_remove_deprecated = true;
        window_info.opengl_profile = WindowInfo::PROFILE::CORE;
        window_info.enable_vsync = false;
        window = Window::create(window_info);

        // components
        std::shared_ptr<SpriteRenderer> sprite_renderer;
        std::shared_ptr<TextureComponent> blue_texture;
        std::shared_ptr<TextureComponent> red_texture;
        std::shared_ptr<TextureComponent> green_texture;
        {
            std::shared_ptr<Shader> shader = resource_manager.build_shader("sprite", "sprite.vert", "sprite.frag");
            sprite_renderer = make_component<SpriteRenderer>(shader);
            blue_texture = make_component<TextureComponent>(TextureChannels{{0, resource_manager.get_texture("blue.png")}});
            red_texture = make_component<TextureComponent>(TextureChannels{{0, resource_manager.get_texture("red.png")}});
            green_texture = make_component<TextureComponent>(TextureChannels{{0, resource_manager.get_texture("green.png")}});
        }

        // background (blue)
        std::shared_ptr<Widget> background_widget = Widget::create();
        background_widget->add_component(sprite_renderer);
        background_widget->add_component(blue_texture);

        // left widget (green)
        std::shared_ptr<Widget> left_widget = Widget::create();
        left_widget->add_component(sprite_renderer);
        left_widget->add_component(green_texture);

        // right widget (red)
        std::shared_ptr<Widget> right_widget = Widget::create();
        right_widget->add_component(sprite_renderer);
        right_widget->add_component(red_texture);

        // horizontal layout
        std::shared_ptr<StackLayout> horizontal_layout = StackLayout::create(StackLayout::DIRECTION::LEFT_TO_RIGHT);
        horizontal_layout->add_item(left_widget);
        horizontal_layout->add_item(right_widget);

        // vertical layout
        std::shared_ptr<StackLayout> vertical_layout = StackLayout::create(StackLayout::DIRECTION::TOP_TO_BOTTOM);
        window->get_layout_root()->set_item(vertical_layout);
        vertical_layout->add_item(horizontal_layout);
        vertical_layout->add_item(background_widget);
    }
    return app.exec();
}
