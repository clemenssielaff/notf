#include "app/render/procedural.hpp"

#include "app/application.hpp"
#include "app/resource_manager.hpp"
#include "app/scene.hpp"
#include "app/window.hpp"
#include "common/log.hpp"
#include "common/system.hpp"
#include "graphics/core/gl_errors.hpp"
#include "graphics/core/graphics_context.hpp"
#include "graphics/core/opengl.hpp"
#include "graphics/core/pipeline.hpp"
#include "graphics/core/shader.hpp"

#include "fmt/format.h"

NOTF_OPEN_NAMESPACE

ProceduralRenderer::ProceduralRenderer(GraphicsContext& context, const std::string& shader_name)
    : Renderer(), m_context(context)
{
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
        vertex_shader = VertexShader::create(context, "__fullscreen.vert", vertex_src);
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
            fragment_shader = FragmentShader::create(context, custom_name, custom_shader_src);
        }
    }

    // create the render pipeline
    m_pipeline = Pipeline::create(context, vertex_shader, fragment_shader);
}

ProceduralRendererPtr ProceduralRenderer::create(Window& window, const std::string& shader_name)
{
    return NOTF_MAKE_SHARED_FROM_PRIVATE(ProceduralRenderer, window.graphics_context(), shader_name);
}

void ProceduralRenderer::_render(valid_ptr<Scene*> scene) const
{
    // match scene properties with shader uniforms
    const FragmentShaderPtr& fragment_shader = m_pipeline->fragment_shader();
    for (const auto& variable : fragment_shader->uniforms()) {
        switch (variable.type) {
        case GL_FLOAT:
            if (PropertyHandle<float> float_property = scene->property<float>(variable.name)) {
                fragment_shader->set_uniform(variable.name, float_property.value());
            }
            break;
        }
    }

    {
        const auto pipeline_guard = m_context.bind_pipeline(m_pipeline);
        notf_check_gl(glDrawArrays(GL_TRIANGLES, 0, 3));
    }
}

NOTF_CLOSE_NAMESPACE
