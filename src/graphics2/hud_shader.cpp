#include "graphics2/hud_shader.hpp"

#include "core/glfw_wrapper.hpp"
#include "graphics2/backend.hpp"

namespace { // anonymous
// clang-format off

// vertex shader source, read from file as described in: http://stackoverflow.com/a/25021520
const char* hud_vertex_shader =
#include "shader/hud.vert"

// fragment shader source
const char* hud_fragment_shader =
#include "shader/hud.frag"

// clang-format on
} // namespace anonymous

namespace notf {

HUDShader::HUDShader(const RenderBackend& backend)
    : m_sources(_create_source(backend))
    , m_shader(Shader::build("HUDShader", m_sources.vertex, m_sources.fragment))
    , m_loc_viewsize(glGetUniformLocation(m_shader.get_id(), "viewSize"))
    , m_loc_texture(glGetUniformLocation(m_shader.get_id(), "tex"))
    , m_loc_buffer(glGetUniformBlockIndex(m_shader.get_id(), "frag"))
    , m_fragment_buffer(0)
    , m_vertex_array(0)
    , m_vertex_buffer(0)
//    , m_frag_size(0)
{
    //  create dynamic vertex arrays
    glGenVertexArrays(1, &m_vertex_array);
    glGenBuffers(1, &m_vertex_buffer);

    int align = 4;
    // create UBOs
    glUniformBlockBinding(m_shader.get_id(), m_loc_buffer, 0);
    glGenBuffers(1, &m_fragment_buffer);
    glGetIntegerv(GL_UNIFORM_BUFFER_OFFSET_ALIGNMENT, &align);
    //    m_frag_size = sizeof(GLNVGfragUniforms) + align - sizeof(GLNVGfragUniforms) % align;

    glFinish();
}

HUDShader::HUDShader(HUDShader&& other)
    : m_sources(std::move(other.m_sources))
    , m_shader(std::move(other.m_shader))
    , m_loc_viewsize(other.m_loc_viewsize)
    , m_loc_texture(other.m_loc_texture)
    , m_loc_buffer(other.m_loc_buffer)
    , m_fragment_buffer(other.m_fragment_buffer)
    , m_vertex_array(other.m_vertex_array)
    , m_vertex_buffer(other.m_vertex_buffer)
//    , m_frag_size(other.m_frag_size)
{
    other.m_fragment_buffer = 0;
    other.m_vertex_array    = 0;
    other.m_vertex_buffer   = 0;
}

HUDShader& HUDShader::operator=(HUDShader&& other)
{
    m_sources       = std::move(other.m_sources);
    m_shader        = std::move(other.m_shader);
    m_loc_viewsize  = other.m_loc_viewsize;
    m_loc_texture   = other.m_loc_texture;
    m_loc_buffer    = other.m_loc_buffer;
    m_vertex_buffer = other.m_vertex_buffer;
//    m_frag_size     = other.m_frag_size;

    other.m_vertex_buffer = 0;

    m_fragment_buffer = other.m_fragment_buffer;
    m_vertex_array    = other.m_vertex_array;

    other.m_fragment_buffer = 0;
    other.m_vertex_array    = 0;

    return *this;
}

HUDShader::~HUDShader()
{
    if (m_fragment_buffer != 0) {
        glDeleteBuffers(1, &m_fragment_buffer);
    }
    if (m_vertex_array != 0) {
        glDeleteVertexArrays(1, &m_vertex_array);
    }
    if (m_vertex_buffer != 0) {
        glDeleteBuffers(1, &m_vertex_buffer);
    }
}

HUDShader::Sources HUDShader::_create_source(const RenderBackend& backend)
{
    // create the header
    std::string header;
    switch (backend.type) {
    case RenderBackend::Type::OPENGL_3:
        header += "#version 150 core\n";
        header += "#define OPENGL_3 1";
        break;
    case RenderBackend::Type::GLES_3:
        header += "#version 300 es\n";
        header += "#define GLES_3 1";
        break;
    }
    if (backend.type != RenderBackend::Type::OPENGL_3) {
        header += "#define UNIFORMARRAY_SIZE 11\n";
    }
    if (!backend.has_msaa) {
        header += "#define GEOMETRY_AA 1\n";
    }
    header += "\n";

    // TODO: building the shader is wasteful (but it only happens once...)

    // attach the header to the source files
    return {header + hud_vertex_shader,
            header + hud_fragment_shader};
}

} // namespace notf
