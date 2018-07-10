#include "auxiliary/visualizer/procedural.hpp"

#include "app/application.hpp"
#include "app/scene.hpp"
#include "app/window.hpp"
#include "common/resource_manager.hpp"
#include "common/system.hpp"
#include "graphics/core/shader.hpp"
#include "graphics/renderer/fragment_renderer.hpp"

#include "fmt/format.h"

NOTF_OPEN_NAMESPACE

ProceduralVisualizer::ProceduralVisualizer(Window& window, const std::string& shader_name) : Visualizer()
{
    GraphicsContext& graphics_context = window.get_graphics_context();
    ResourceManager& resource_manager = ResourceManager::get_instance();
    auto& vs_type = resource_manager.get_type<VertexShader>();
    auto& fs_type = resource_manager.get_type<FragmentShader>();

    // load or get the fullscreen vertex shader
    static const std::string vertex_shader_name = "__fullscreen.vert";
    ResourceHandle<VertexShader> vertex_shader = vs_type.get(vertex_shader_name);
    if (!vertex_shader) {
        const std::string vertex_src = load_file(fmt::format("{}fullscreen.vert", vs_type.get_path()));
        vertex_shader
            = vs_type.set(vertex_shader_name, VertexShader::create(graphics_context, vertex_shader_name, vertex_src));
    }

    // load or get the custom fragment shader.
    const std::string fragment_shader_name = fmt::format("__procedural_{}", shader_name);
    ResourceHandle<FragmentShader> fragment_shader = fs_type.get(fragment_shader_name);
    if (!fragment_shader) {
        const std::string fragment_src = load_file(fs_type.get_path() + shader_name);
        fragment_shader = fs_type.set(fragment_shader_name,
                                      FragmentShader::create(graphics_context, fragment_shader_name, fragment_src));
    }

    // create the renderer
    m_renderer = std::make_unique<FragmentRenderer>(vertex_shader.get_shared(), fragment_shader.get_shared());
}

ProceduralVisualizer::~ProceduralVisualizer() = default;

void ProceduralVisualizer::visualize(valid_ptr<Scene*> scene) const
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
