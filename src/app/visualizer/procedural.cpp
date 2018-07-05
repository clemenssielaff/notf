#include "app/visualizer/procedural.hpp"

#include "app/application.hpp"
#include "app/resource_manager.hpp"
#include "app/scene.hpp"
#include "app/window.hpp"
#include "common/system.hpp"
#include "graphics/core/shader.hpp"
#include "graphics/renderer/fragment_renderer.hpp"

#include "fmt/format.h"

NOTF_OPEN_NAMESPACE

ProceduralVisualizer::ProceduralVisualizer(Window& window, const std::string& shader_name) : Visualizer()
{
    GraphicsContext& graphics_context = window.get_graphics_context();
    ResourceManager& resource_manager = Application::instance().resource_manager();

    // load or get the fullscreen vertex shader
    VertexShaderPtr vertex_shader;
    {
        risky_ptr<ShaderPtr> fullscreen_shader = resource_manager.shader("__fullscreen.vert");
        if (fullscreen_shader) {
            vertex_shader = std::dynamic_pointer_cast<VertexShader>(fullscreen_shader.get());
        }
    }
    if (!vertex_shader) {
        const std::string vertex_src = load_file(fmt::format("{}fullscreen.vert", resource_manager.shader_directory()));
        vertex_shader = VertexShader::create(graphics_context, "__fullscreen.vert", vertex_src);
    }

    // load or get the custom fragment shader.
    FragmentShaderPtr fragment_shader;
    {
        const std::string custom_name = fmt::format("__procedural_{}", shader_name);
        risky_ptr<ShaderPtr> stored_shader = resource_manager.shader(custom_name);
        if (stored_shader) {
            fragment_shader = std::dynamic_pointer_cast<FragmentShader>(stored_shader.get());
        }
        if (!fragment_shader) {
            const std::string custom_shader_src = load_file(resource_manager.shader_directory() + shader_name);
            fragment_shader = FragmentShader::create(graphics_context, custom_name, custom_shader_src);
        }
    }

    // create the renderer
    m_renderer = std::make_unique<FragmentRenderer>(std::move(vertex_shader), std::move(fragment_shader));
}

ProceduralVisualizer::~ProceduralVisualizer() = default;

void ProceduralVisualizer::_visualize(valid_ptr<Scene*> scene) const
{
    // match scene properties with shader uniforms
    for (const auto& variable : m_renderer->get_uniforms()) {
        switch (variable.type) {
        case GL_FLOAT:
            if (PropertyHandle<float> float_property = scene->get_property<float>(variable.name)) {
                m_renderer->set_uniform(variable.name, float_property.get());
            }
            break;
        }
    }

    m_renderer->render();
}

NOTF_CLOSE_NAMESPACE
