#include <glad/glad.h>
#include <glm/gtc/matrix_transform.hpp>

#include "core/application.hpp"
#include "core/components/texture_component.hpp"
#include "core/widget.hpp"
#include "core/window.hpp"
#include "graphics/sprite_component.hpp"

using namespace signal;

void background_setup(Shader& shader, const Window& window)
{
    Size2 size = window.get_canvas_size();
    glm::mat4 projection = glm::ortho(0.0f, static_cast<GLfloat>(size.width), static_cast<GLfloat>(size.height), 0.0f, -1.0f, 1.0f);
    shader.set_uniform("image", 0);
    shader.set_uniform("projection", projection);
}

void background_renderer(Shader& shader, const Widget& widget)
{
    Size2 canvas_size = widget.get_window()->get_canvas_size();

    glm::mat4 model;
    glm::vec2 position(0, 0);
    GLfloat rotate = 0.0f;
    glm::vec2 size(canvas_size.width, canvas_size.height);
    glm::vec3 color(1.0f);

    model = glm::translate(model, glm::vec3(position, 0.0f)); // First translate (transformations are: scale happens first, then rotation and then finall translation happens; reversed order)

    model = glm::translate(model, glm::vec3(0.5f * size.x, 0.5f * size.y, 0.0f)); // Move origin of rotation to center of quad
    model = glm::rotate(model, rotate, glm::vec3(0.0f, 0.0f, 1.0f)); // Then rotate
    model = glm::translate(model, glm::vec3(-0.5f * size.x, -0.5f * size.y, 0.0f)); // Move origin back

    model = glm::scale(model, glm::vec3(size, 1.0f)); // Last scale

    shader.set_uniform("model", model);

    // Render textured quad
    shader.set_uniform("spriteColor", color);
}

int main(void)
{
    Application& app = Application::get_instance();
    ResourceManager& resource_manager = app.get_resource_manager();
    resource_manager.set_texture_directory("/home/clemens/code/signal-ui/res/textures");
    resource_manager.set_shader_directory("/home/clemens/code/signal-ui/res/shaders");

    WindowInfo window_info;
//    window_info.enable_vsync = false; // to correctly time each frame
    window_info.width = 800;
    window_info.height = 600;
//    window_info.is_resizeable = false;
    Window window(window_info);

    std::shared_ptr<Widget> widget = Widget::make_widget();
    widget->set_parent(window.get_root_widget());

    auto shader = resource_manager.build_shader("sprite", "sprite.vert", "sprite.frag");
    shader->set_window_setup(background_setup);
    shader->set_widget_setup(background_renderer);
    widget->add_component(make_component<SpriteComponent>(shader));

    auto texture = resource_manager.get_texture("background.jpg");
    auto texture_component = make_component<TextureComponent>();
    texture_component->set_texture(0, texture);
    widget->add_component(texture_component);

    return app.exec();
}
