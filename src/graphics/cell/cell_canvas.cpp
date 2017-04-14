#include "graphics/cell/cell_canvas.hpp"

#include <sstream>

#include "common/log.hpp"
#include "core/opengl.hpp"
#include "graphics/gl_errors.hpp"
#include "graphics/gl_utils.hpp"
#include "graphics/graphics_context.hpp"
#include "graphics/shader.hpp"
#include "graphics/texture2.hpp"
#include "utils/enum_to_number.hpp"

namespace { // anonymous
// clang-format off

// vertex shader source, read from file as described in: http://stackoverflow.com/a/25021520
const char* cell_vertex_shader =
#include "shader/cell.vert"

// fragment shader source
const char* cell_fragment_shader =
#include "shader/cell.frag"
} // namespace anonymous

namespace { // anonymous
using namespace notf;

/** Loads the shader sources and creates a dynamic header for them. */
std::pair<std::string, std::string> create_shader_sources(const GraphicsContext& context)
{
    // create the header
    std::stringstream ss;
    ss << "#version 300 es\n";
    if (context.get_options().geometric_aa) {
        ss << "#define GEOMETRY_AA 1\n";
    }
    if(context.get_options().stencil_strokes){
        ss << "#define SAVE_ALPHA_STROKE 1\n";
    }
    ss << "\n";
    std::string header = ss.str();

    // attach the header to the source files
    return {header + cell_vertex_shader,
            header + cell_fragment_shader};
}

const GLuint FRAG_BINDING = 0;

} // namespace anonymous

namespace notf {

CellCanvas::CellCanvas(GraphicsContext& context)
    : m_graphics_context(context)
    , m_painterpreter(std::make_unique<Painterpreter>(*this))
    , m_options()
    , m_cell_shader()
    , m_calls()
    , m_paths()
    , m_vertices()
    , m_shader_variables()
    , m_fragment_buffer(0)
    , m_vertex_array(0)
    , m_vertex_buffer(0)
{
    // create the CellShader
    const std::pair<std::string, std::string> sources = create_shader_sources(m_graphics_context);
    m_cell_shader.shader        = Shader::build(m_graphics_context, "CellShader", sources.first, sources.second);
    const GLuint cell_shader_id = m_cell_shader.shader->get_id();
    m_cell_shader.viewsize      = glGetUniformLocation(cell_shader_id, "viewSize");
    m_cell_shader.image         = glGetUniformLocation(cell_shader_id, "image");
    m_cell_shader.variables     = glGetUniformBlockIndex(cell_shader_id, "variables");

    // create dynamic vertex arrays
    glGenVertexArrays(1, &m_vertex_array);
    glGenBuffers(1, &m_vertex_buffer);

    // create UBOs
    glUniformBlockBinding(cell_shader_id, m_cell_shader.variables, 0);
    glGenBuffers(1, &m_fragment_buffer);
    GLint align = sizeof(float);
    glGetIntegerv(GL_UNIFORM_BUFFER_OFFSET_ALIGNMENT, &align);

    glFinish();
    check_gl_error();
}

CellCanvas::~CellCanvas()
{
    if (m_fragment_buffer != 0) {
        glDeleteBuffers(1, &m_fragment_buffer);
        m_fragment_buffer = 0;
    }
    if (m_vertex_array != 0) {
        glDeleteVertexArrays(1, &m_vertex_array);
        m_vertex_array = 0;
    }
    if (m_vertex_buffer != 0) {
        glDeleteBuffers(1, &m_vertex_buffer);
        m_vertex_buffer = 0;
    }
}

const FontManager& CellCanvas::get_font_manager() const
{
    return m_graphics_context.get_font_manager();
}

void CellCanvas::begin_frame(const Size2i& buffer_size, const Time time, const Vector2f mouse_pos)
{
    reset();

    m_options.distance_tolerance = 0.01f / m_graphics_context.get_options().pixel_ratio;
    m_options.tesselation_tolerance = 0.25f / m_graphics_context.get_options().pixel_ratio;
    m_options.fringe_width = 1.f / m_graphics_context.get_options().pixel_ratio;

    m_options.geometric_aa = m_graphics_context.get_options().geometric_aa;
    m_options.stencil_strokes = m_graphics_context.get_options().stencil_strokes;

    m_options.buffer_size.width  = static_cast<float>(buffer_size.width);
    m_options.buffer_size.height = static_cast<float>(buffer_size.height);

    m_options.mouse_pos = std::move(mouse_pos);

    m_options.time = time;
}

void CellCanvas::reset()
{
    m_calls.clear();
    m_paths.clear();
    m_vertices.clear();
    m_shader_variables.clear();
}

void CellCanvas::finish_frame()
{
    if (m_calls.empty()) {
        return;
    }
    m_graphics_context.force_reloads();

    // setup GL state
    m_cell_shader.shader->bind();
    m_graphics_context.set_blend_mode(BlendMode::SOURCE_OVER);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glFrontFace(GL_CCW);
    glEnable(GL_BLEND);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_SCISSOR_TEST);
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    m_graphics_context.set_stencil_mask(0xff);
    glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
    m_graphics_context.set_stencil_func(StencilFunc::ALWAYS);
    glActiveTexture(GL_TEXTURE0);
    Texture2::unbind();

    // upload ubo for frag shaders
    glBindBuffer(GL_UNIFORM_BUFFER, m_fragment_buffer);
    glBufferData(GL_UNIFORM_BUFFER, static_cast<GLsizeiptr>(m_shader_variables.size()) * fragmentSize(), &m_shader_variables.front(), GL_STREAM_DRAW);

    // upload vertex data
    glBindVertexArray(m_vertex_array);
    glBindBuffer(GL_ARRAY_BUFFER, m_vertex_buffer);
    glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(m_vertices.size() * sizeof(Vertex)), &m_vertices.front(), GL_STREAM_DRAW);
    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), nullptr);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), gl_buffer_offset(2 * sizeof(float)));

    // Set view and texture just once per frame.
    glUniform1i(m_cell_shader.image, 0);
    glUniform2fv(m_cell_shader.viewsize, 1, m_options.buffer_size.as_ptr());

    // perform the render calls
    for (const Call& call : m_calls) {
        switch (call.type) {
        case Call::Type::FILL:
            _perform_fill(call);
            break;
        case Call::Type::CONVEX_FILL:
            _perform_convex_fill(call);
            break;
        case Call::Type::STROKE:
            _perform_stroke(call);
            break;
        case Call::Type::TEXT:
            _render_text(call);
            break;
        }
    }

    // teardown GL state
    glDisableVertexAttribArray(0);
    glDisableVertexAttribArray(1);
    glBindVertexArray(0);
    glDisable(GL_CULL_FACE);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    Shader::unbind();

    check_gl_error();
}

void CellCanvas::_perform_convex_fill(  const Call& call)
{
    assert(call.path_offset + call.path_count <= m_paths.size());

    glBindBufferRange(GL_UNIFORM_BUFFER, FRAG_BINDING, m_fragment_buffer, call.uniform_offset, fragmentSize());
    if (call.texture) {
        call.texture->bind();
    }

    for (size_t i = call.path_offset; i < call.path_offset + call.path_count; ++i) {
        // draw fill
        assert(static_cast<size_t>(m_paths[i].fill_offset + m_paths[i].fill_count) <= m_vertices.size());
        glDrawArrays(GL_TRIANGLE_FAN, m_paths[i].fill_offset, m_paths[i].fill_count);
    }
    if (m_graphics_context.get_options().geometric_aa) {
        // draw fringes
        for (size_t i = call.path_offset; i < call.path_offset + call.path_count; ++i) {
            assert(static_cast<size_t>(m_paths[i].stroke_offset + m_paths[i].stroke_count) <= m_vertices.size());
            glDrawArrays(GL_TRIANGLE_STRIP, m_paths[i].stroke_offset, m_paths[i].stroke_count);
        }
    }

    check_gl_error();
}

void CellCanvas::_perform_fill(const Call& call)
{
    assert(call.path_offset + call.path_count <= m_paths.size());
    assert(static_cast<size_t>(call.uniform_offset) <= max(size_t(0), m_shader_variables.size() - 1) * fragmentSize());

    // draw stencil shapes
    glEnable(GL_STENCIL_TEST);
    m_graphics_context.set_stencil_mask(0xff);
    m_graphics_context.set_stencil_func(StencilFunc::ALWAYS);
    glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);

    // set bindpoint for solid loc
    glBindBufferRange(GL_UNIFORM_BUFFER, FRAG_BINDING, m_fragment_buffer, call.uniform_offset, fragmentSize());

    glStencilOpSeparate(GL_FRONT, GL_KEEP, GL_KEEP, GL_INCR_WRAP);
    glStencilOpSeparate(GL_BACK, GL_KEEP, GL_KEEP, GL_DECR_WRAP);
    glDisable(GL_CULL_FACE);
    for (size_t i = call.path_offset; i < call.path_offset + call.path_count; ++i) {
        assert(static_cast<size_t>(m_paths[i].fill_offset + m_paths[i].fill_count) <= m_vertices.size());
        glDrawArrays(GL_TRIANGLE_FAN, m_paths[i].fill_offset, m_paths[i].fill_count);
    }
    glEnable(GL_CULL_FACE);

    // draw anti-aliased pixels
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    glBindBufferRange(GL_UNIFORM_BUFFER, FRAG_BINDING, m_fragment_buffer, call.uniform_offset + fragmentSize(), fragmentSize());
    if (call.texture) {
        call.texture->bind();
    }

    if (m_graphics_context.get_options().geometric_aa) {
        m_graphics_context.set_stencil_func(StencilFunc::EQUAL);
        glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
        // draw fringes
        for (size_t i = call.path_offset; i < call.path_offset + call.path_count; ++i) {
            assert(static_cast<size_t>(m_paths[i].stroke_offset + m_paths[i].stroke_count) <= m_vertices.size());
            glDrawArrays(GL_TRIANGLE_STRIP, m_paths[i].stroke_offset, m_paths[i].stroke_count);
        }
    }

    // Draw fill
    m_graphics_context.set_stencil_func(StencilFunc::NOTEQUAL);
    glStencilOp(GL_ZERO, GL_ZERO, GL_ZERO);
    glDrawArrays(GL_TRIANGLES, call.polygon_offset, /* triangle count = */ 6);

    glDisable(GL_STENCIL_TEST);

    check_gl_error();
}

void CellCanvas::_perform_stroke(const Call& call)
{
    assert(call.path_offset + call.path_count <= m_paths.size());
    assert(static_cast<size_t>(call.uniform_offset) <= max(size_t(0), m_shader_variables.size() - 1) * fragmentSize());

    if(m_graphics_context.get_options().stencil_strokes){
        glEnable(GL_STENCIL_TEST);
        m_graphics_context.set_stencil_mask(0xff);

        // fill the stroke base without overlap
        m_graphics_context.set_stencil_func(StencilFunc::EQUAL);
        glStencilOp(GL_KEEP, GL_KEEP, GL_INCR);
        glBindBufferRange(GL_UNIFORM_BUFFER, FRAG_BINDING, m_fragment_buffer, call.uniform_offset + fragmentSize(), fragmentSize());
        if (call.texture) {
            call.texture->bind();
        }
        for (size_t i = call.path_offset; i < call.path_offset + call.path_count; ++i) {
            assert(static_cast<size_t>(m_paths[i].stroke_offset + m_paths[i].stroke_count) <= m_vertices.size());
            glDrawArrays(GL_TRIANGLE_STRIP, m_paths[i].stroke_offset, m_paths[i].stroke_count);
        }

        // draw anti-aliased pixels
        glBindBufferRange(GL_UNIFORM_BUFFER, FRAG_BINDING, m_fragment_buffer, call.uniform_offset, fragmentSize());
        if (call.texture) {
            call.texture->bind();
        }
        m_graphics_context.set_stencil_func(StencilFunc::EQUAL);
        glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
        for (size_t i = call.path_offset; i < call.path_offset + call.path_count; ++i) {
            assert(static_cast<size_t>(m_paths[i].stroke_offset + m_paths[i].stroke_count) <= m_vertices.size());
            glDrawArrays(GL_TRIANGLE_STRIP, m_paths[i].stroke_offset, m_paths[i].stroke_count);
        }

        // clear stencil buffer
        glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
        m_graphics_context.set_stencil_func(StencilFunc::ALWAYS);
        glStencilOp(GL_ZERO, GL_ZERO, GL_ZERO);
        for (size_t i = call.path_offset; i < call.path_offset + call.path_count; ++i) {
            assert(static_cast<size_t>(m_paths[i].stroke_offset + m_paths[i].stroke_count) <= m_vertices.size());
            glDrawArrays(GL_TRIANGLE_STRIP, m_paths[i].stroke_offset, m_paths[i].stroke_count);
        }
        glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);

        glDisable(GL_STENCIL_TEST);

    } else { // !m_context.get_args().save_alpha_stroke
        glBindBufferRange(GL_UNIFORM_BUFFER, FRAG_BINDING, m_fragment_buffer, call.uniform_offset, fragmentSize());
        if (call.texture) {
            call.texture->bind();
        }
        for (size_t i = call.path_offset; i < call.path_offset + call.path_count; ++i) {
            glDrawArrays(GL_TRIANGLE_STRIP, m_paths[i].stroke_offset, m_paths[i].stroke_count);
        }
        return;
    }

    check_gl_error();
}

void CellCanvas::_render_text(const Call& call)
{
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glBindBufferRange(GL_UNIFORM_BUFFER, FRAG_BINDING, m_fragment_buffer, call.uniform_offset, fragmentSize());
    call.texture->bind();
    glDrawArrays(GL_TRIANGLES, call.polygon_offset, call.polygon_count);
    check_gl_error();
}

void CellCanvas::_dump_debug_info() const
{
    log_format << "==========================================================\n"
               << "== Render Context Dump Begin                            ==\n"
               << "==========================================================";

    log_format << "==========================================================\n"
               << "== Arguments                                            ==";
    log_trace << "Enable geometric AA: " << m_graphics_context.get_options().geometric_aa;
    log_trace << "Pixel ratio: " << m_graphics_context.get_options().pixel_ratio;

    log_format << "==========================================================\n"
               << "== Calls                                                ==";
    for(const Call& call : m_calls){
        log_trace << "Type: " << static_cast<int>(to_number(call.type)) << " | "
                  << "Path offset:" << call.path_offset << ", Path count: " << call.path_count << " | "
                  << "Uniform offset: " << call.uniform_offset << " | "
                  << "Texture: " << call.texture << " | "
                  << "Polygon offset: " << call.polygon_offset;
    }

    log_format << "==========================================================\n"
               << "== Paths                                                ==";
    for(const Path& path : m_paths){
        log_trace << "Fill offset: " << path.fill_offset << ", Fill count:" << path.fill_count << " | "
                  << "Stroke offset: " << path.stroke_offset << ", Stroke count" << path.stroke_count;
    }

    log_format << "==========================================================\n"
               << "== Vertices                                             ==";
    for(const Vertex& vertex : m_vertices){
        log_trace << "Pos: " << vertex.pos.x << ", " << vertex.pos.y << " | "
                  << "UV: "<< vertex.uv.x << ", " << vertex.uv.y;
    }

    log_format << "==========================================================\n"
               << "== Render Context Dump End                              ==\n"
               << "==========================================================";
}


} // namespace notf
