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

void blub(std::shared_ptr<Window> window)
{
    log_info << "HIT IT!";

    ResourceManager& resource_manager = Application::get_instance().get_resource_manager();

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

int main(int argc, char* argv[])
{
    Application& app = Application::initialize(argc, argv);

    // resource manager
    ResourceManager& resource_manager = app.get_resource_manager();
    resource_manager.set_texture_directory("/home/clemens/code/notf/res/textures");
    resource_manager.set_shader_directory("/home/clemens/code/notf/res/shaders");

    // window
    WindowMake window_make;
    window_make.width = 800;
    window_make.height = 600;
    window_make.opengl_version_major = 3;
    window_make.opengl_version_minor = 3;
    window_make.opengl_remove_deprecated = true;
    window_make.opengl_profile = WindowMake::PROFILE::CORE;
    window_make.enable_vsync = false;
    std::shared_ptr<Window> window = Window::create(window_make);

    window->on_token_key.connect(
        [window, &app](const KeyEvent&) {
            PythonInterpreter& python = app.get_python_interpreter();
            python.parse_app();
            //            blub(window);

        },
        [](const KeyEvent& event) -> bool {
            return event.key == KEY::SPACE && event.action == KEY_ACTION::KEY_PRESS;
        });

    return app.exec();
}
