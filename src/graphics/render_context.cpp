#include "graphics/render_context.hpp"

#include "common/aabr.hpp"
#include "common/log.hpp"
#include "common/vector.hpp"
#include "common/xform2.hpp"
#include "core/glfw.hpp"
#include "core/window.hpp"
#include "graphics/blend_mode.hpp"
#include "graphics/cell/cell.hpp"
#include "graphics/cell/painterpreter.hpp"
#include "graphics/gl_utils.hpp"
#include "graphics/scissor.hpp"
#include "graphics/shader.hpp"
#include "graphics/texture2.hpp"

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
std::pair<std::string, std::string> create_shader_sources(const RenderContext& context)
{
    // create the header
    std::stringstream ss;
    ss << "#version 300 es\n"
       << "#define UNIFORMARRAY_SIZE 11\n"; // TODO: why is UNIFORMARRAY_SIZE == 11 ?
    if (context.provides_geometric_aa()) {
        ss << "#define GEOMETRY_AA 1\n";
    }
    ss << "\n";
    std::string header = ss.str();

    // attach the header to the source files
    return {header + cell_vertex_shader,
            header + cell_fragment_shader};
}

void xformToMat3x4(float* m3, Xform2f t)
{
    m3[0]  = t[0][0];
    m3[1]  = t[0][1];
    m3[2]  = 0.0f;
    m3[3]  = 0.0f;
    m3[4]  = t[1][0];
    m3[5]  = t[1][1];
    m3[6]  = 0.0f;
    m3[7]  = 0.0f;
    m3[8]  = t[2][0];
    m3[9]  = t[2][1];
    m3[10] = 1.0f;
    m3[11] = 0.0f;
}

const GLuint FRAG_BINDING = 0;

} // namespace anonymous

namespace notf {

RenderContext* RenderContext::s_current_context = nullptr;

RenderContext::RenderContext(const Window* window, const RenderContextArguments args)
    : m_window(window)
    , m_args(std::move(args))
    , m_painterpreter(std::make_unique<Painterpreter>(*this))
    , m_calls()
    , m_paths()
    , m_vertices()
    , m_shader_variables()
    , m_distance_tolerance(0.01f / m_args.pixel_ratio)
    , m_tesselation_tolerance(0.25f / m_args.pixel_ratio)
    , m_fringe_width(1.f / m_args.pixel_ratio)
    , m_buffer_size(Size2f(0, 0))
    , m_time()
    , m_mouse_pos(Vector2f::zero())
    , m_stencil_func(StencilFunc::ALWAYS)
    , m_stencil_mask(0)
    , m_blend_mode(BlendMode::SOURCE_OVER)
    , m_bound_texture(0)
    , m_textures()
    , m_bound_shader(0)
    , m_shaders()
    , m_cell_shader()
    , m_fragment_buffer(0)
    , m_vertex_array(0)
    , m_vertex_buffer(0)
{
    // make sure the pixel ratio is real and never zero
    if (!is_real(m_args.pixel_ratio)) {
        log_warning << "Pixel ratio cannot be " << m_args.pixel_ratio << ", defaulting to 1";
        m_args.pixel_ratio = 1;
    }
    else if (std::abs(m_args.pixel_ratio) < precision_high<float>()) {
        log_warning << "Pixel ratio cannot be zero, defaulting to 1";
        m_args.pixel_ratio = 1;
    }

    // create the CellShader
    const std::pair<std::string, std::string> sources = create_shader_sources(*this);
    m_cell_shader.shader        = build_shader("CellShader", sources.first, sources.second);
    const GLuint cell_shader_id = m_cell_shader.shader->get_id();
    m_cell_shader.viewsize      = glGetUniformLocation(cell_shader_id, "viewSize");
    m_cell_shader.texture       = glGetUniformLocation(cell_shader_id, "tex");
    m_cell_shader.variables     = glGetUniformBlockIndex(cell_shader_id, "frag");

    // create dynamic vertex arrays
    glGenVertexArrays(1, &m_vertex_array);
    glGenBuffers(1, &m_vertex_buffer);

    // create UBOs
    glUniformBlockBinding(cell_shader_id, m_cell_shader.variables, 0);
    glGenBuffers(1, &m_fragment_buffer);
    GLint align = sizeof(float);
    glGetIntegerv(GL_UNIFORM_BUFFER_OFFSET_ALIGNMENT, &align);

    glFinish();
}

RenderContext::~RenderContext()
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

    // deallocate and invalidate all remaining Textures
    for (std::weak_ptr<Texture2> texture_weakptr : m_textures) {
        std::shared_ptr<Texture2> texture = texture_weakptr.lock();
        if (texture) {
            log_warning << "Deallocating live Texture: " << texture->get_name();
            texture->_deallocate();
        }
    }
    m_textures.clear();

    // deallocate and invalidate all remaining Shaders
    for (std::weak_ptr<Shader> shader_weakptr : m_shaders) {
        std::shared_ptr<Shader> shader = shader_weakptr.lock();
        if (shader) {
            log_warning << "Deallocating live Shader: " << shader->get_name();
            shader->_deallocate();
        }
    }
    m_shaders.clear();
}

void RenderContext::make_current()
{
    if (s_current_context != this) {
        glfwMakeContextCurrent(m_window->_get_glfw_window());
        s_current_context = this;
    }
}

void RenderContext::set_stencil_func(const StencilFunc func)
{
    if (func != m_stencil_func) {
        m_stencil_func = func;
        m_stencil_func.apply();
    }
}

void RenderContext::set_stencil_mask(const GLuint mask)
{
    if (mask != m_stencil_mask) {
        m_stencil_mask = mask;
        glStencilMask(mask);
    }
}

void RenderContext::set_blend_mode(const BlendMode mode)
{
    if (mode != m_blend_mode) {
        m_blend_mode = mode;
        m_blend_mode.apply();
    }
}

std::shared_ptr<Texture2> RenderContext::load_texture(const std::string& file_path)
{
    std::shared_ptr<Texture2> texture = Texture2::load(this, file_path);
    if (texture) {
        m_textures.emplace_back(texture);
    }
    return texture;
}

std::shared_ptr<Shader> RenderContext::build_shader(const std::string& name,
                                                    const std::string& vertex_shader_source,
                                                    const std::string& fragment_shader_source)
{
    std::shared_ptr<Shader> shader = Shader::build(this, name, vertex_shader_source, fragment_shader_source);
    if (shader) {
        m_shaders.emplace_back(shader);
    }
    return shader;
}

void RenderContext::_begin_frame(const Size2i& buffer_size)
{
    _reset();

    m_buffer_size.width  = static_cast<float>(buffer_size.width);
    m_buffer_size.height = static_cast<float>(buffer_size.height);

    m_time = Time::now();
}

void RenderContext::_reset()
{
    m_calls.clear();
    m_paths.clear();
    m_vertices.clear();
    m_shader_variables.clear();
}

void RenderContext::_finish_frame()
{
    if (m_calls.empty()) {
        return;
    }

    // reset cache
    m_stencil_func  = StencilFunc::INVALID;
    m_stencil_mask  = 0;
    m_blend_mode    = BlendMode::INVALID;
    m_bound_texture = 0;
    m_bound_shader  = 0;

    // setup GL state
    m_cell_shader.shader->bind();
    set_blend_mode(BlendMode::SOURCE_OVER);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glFrontFace(GL_CCW);
    glEnable(GL_BLEND);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_SCISSOR_TEST);
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    set_stencil_mask(0xffffffff);
    glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
    set_stencil_func(StencilFunc::ALWAYS);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, 0);

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
    glUniform1i(m_cell_shader.texture, 0);
    glUniform2fv(m_cell_shader.viewsize, 1, m_buffer_size.as_ptr());

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
        }
    }

    // teardown GL state
    glDisableVertexAttribArray(0);
    glDisableVertexAttribArray(1);
    glBindVertexArray(0);
    glDisable(GL_CULL_FACE);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    Shader::unbind();
}

void RenderContext::_bind_texture(const GLuint texture_id)
{
    if (texture_id != m_bound_texture) {
        glBindTexture(GL_TEXTURE_2D, texture_id);
        m_bound_texture = texture_id;
    }
}

void RenderContext::_bind_shader(const GLuint shader_id)
{
    if (shader_id != m_bound_shader) {
        glUseProgram(shader_id);
        m_bound_shader = shader_id;
    }
}

void RenderContext::_perform_convex_fill(const Call& call)
{
    assert(call.path_offset + call.path_count <= m_paths.size());

    glBindBufferRange(GL_UNIFORM_BUFFER, FRAG_BINDING, m_fragment_buffer, call.uniform_offset, fragmentSize());
    if (call.texture) {
        _bind_texture(call.texture->get_id());
    }

    for (size_t i = call.path_offset; i < call.path_offset + call.path_count; ++i) {
        // draw fill
        assert(static_cast<size_t>(m_paths[i].fill_offset + m_paths[i].fill_count) <= m_vertices.size());
        glDrawArrays(GL_TRIANGLE_FAN, m_paths[i].fill_offset, m_paths[i].fill_count);
    }
    if (m_args.enable_geometric_aa) {
        // draw fringes
        for (size_t i = call.path_offset; i < call.path_offset + call.path_count; ++i) {
            assert(static_cast<size_t>(m_paths[i].stroke_offset + m_paths[i].stroke_count) <= m_vertices.size());
            glDrawArrays(GL_TRIANGLE_STRIP, m_paths[i].stroke_offset, m_paths[i].stroke_count);
        }
    }
}

void RenderContext::_perform_fill(const Call& call)
{
    assert(call.path_offset + call.path_count <= m_paths.size());
    assert(static_cast<size_t>(call.uniform_offset) <= max(size_t(0), m_shader_variables.size() - 1) * fragmentSize());

    // draw stencil shapes
    glEnable(GL_STENCIL_TEST);
    set_stencil_mask(0xff);
    set_stencil_func(StencilFunc::ALWAYS);
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
        _bind_texture(call.texture->get_id());
    }

    if (m_args.enable_geometric_aa) {
        set_stencil_func(StencilFunc::EQUAL);
        glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
        // draw fringes
        for (size_t i = call.path_offset; i < call.path_offset + call.path_count; ++i) {
            assert(static_cast<size_t>(m_paths[i].stroke_offset + m_paths[i].stroke_count) <= m_vertices.size());
            glDrawArrays(GL_TRIANGLE_STRIP, m_paths[i].stroke_offset, m_paths[i].stroke_count);
        }
    }

    // Draw fill
    set_stencil_func(StencilFunc::NOTEQUAL);
    glStencilOp(GL_ZERO, GL_ZERO, GL_ZERO);
    glDrawArrays(GL_TRIANGLES, call.polygon_offset, /* triangle count = */ 6);

    glDisable(GL_STENCIL_TEST);
}

void RenderContext::_perform_stroke(const Call& call)
{
    assert(call.path_offset + call.path_count <= m_paths.size());
    assert(static_cast<size_t>(call.uniform_offset) <= max(size_t(0), m_shader_variables.size() - 1) * fragmentSize());

    glEnable(GL_STENCIL_TEST);
    set_stencil_mask(0xff);

    // fill the stroke base without overlap
    set_stencil_func(StencilFunc::EQUAL);
    glStencilOp(GL_KEEP, GL_KEEP, GL_INCR);
    glBindBufferRange(GL_UNIFORM_BUFFER, FRAG_BINDING, m_fragment_buffer, call.uniform_offset + fragmentSize(), fragmentSize());
    if (call.texture) {
        _bind_texture(call.texture->get_id());
    }
    for (size_t i = call.path_offset; i < call.path_offset + call.path_count; ++i) {
        assert(static_cast<size_t>(m_paths[i].stroke_offset + m_paths[i].stroke_count) <= m_vertices.size());
        glDrawArrays(GL_TRIANGLE_STRIP, m_paths[i].stroke_offset, m_paths[i].stroke_count);
    }

    // draw anti-aliased pixels
    glBindBufferRange(GL_UNIFORM_BUFFER, FRAG_BINDING, m_fragment_buffer, call.uniform_offset, fragmentSize());
    if (call.texture) {
        _bind_texture(call.texture->get_id());
    }
    set_stencil_func(StencilFunc::EQUAL);
    glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
    for (size_t i = call.path_offset; i < call.path_offset + call.path_count; ++i) {
        assert(static_cast<size_t>(m_paths[i].stroke_offset + m_paths[i].stroke_count) <= m_vertices.size());
        glDrawArrays(GL_TRIANGLE_STRIP, m_paths[i].stroke_offset, m_paths[i].stroke_count);
    }

    // clear stencil buffer
    glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
    set_stencil_func(StencilFunc::ALWAYS);
    glStencilOp(GL_ZERO, GL_ZERO, GL_ZERO);
    for (size_t i = call.path_offset; i < call.path_offset + call.path_count; ++i) {
        assert(static_cast<size_t>(m_paths[i].stroke_offset + m_paths[i].stroke_count) <= m_vertices.size());
        glDrawArrays(GL_TRIANGLE_STRIP, m_paths[i].stroke_offset, m_paths[i].stroke_count);
    }
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);

    glDisable(GL_STENCIL_TEST);
}

void paint_to_frag(RenderContext::ShaderVariables& frag, const Paint& paint, const Scissor& scissor,
                              const float stroke_width, const float fringe, const float stroke_threshold)
{
    assert(fringe > 0);

    // TODO: I don't know if scissor is ever anything else than identiy & -1/-1

    frag.innerCol = paint.inner_color.premultiplied();
    frag.outerCol = paint.outer_color.premultiplied();
    if (scissor.extend.width < 1.0f || scissor.extend.height < 1.0f) { // extend cannot be less than a pixel
        frag.scissorExt[0]   = 1.0f;
        frag.scissorExt[1]   = 1.0f;
        frag.scissorScale[0] = 1.0f;
        frag.scissorScale[1] = 1.0f;
    }
    else {
        xformToMat3x4(frag.scissorMat, scissor.xform.get_inverse());
        frag.scissorExt[0]   = scissor.extend.width / 2;
        frag.scissorExt[1]   = scissor.extend.height / 2;
        frag.scissorScale[0] = sqrt(scissor.xform[0][0] * scissor.xform[0][0] + scissor.xform[1][0] * scissor.xform[1][0]) / fringe;
        frag.scissorScale[1] = sqrt(scissor.xform[0][1] * scissor.xform[0][1] + scissor.xform[1][1] * scissor.xform[1][1]) / fringe;
    }
    frag.extent[0]  = paint.extent.width;
    frag.extent[1]  = paint.extent.height;
    frag.strokeMult = (stroke_width * 0.5f + fringe * 0.5f) / fringe;
    frag.strokeThr  = stroke_threshold;

    if (paint.texture) {
        frag.type    = RenderContext::ShaderVariables::Type::IMAGE;
        frag.texType = paint.texture->get_format() == Texture2::Format::GRAYSCALE ? 2 : 0; // TODO: change the 'texType' uniform in the shader to something more meaningful
    }
    else { // no image
        frag.type    = RenderContext::ShaderVariables::Type::GRADIENT;
        frag.radius  = paint.radius;
        frag.feather = paint.feather;
    }
    xformToMat3x4(frag.paintMat, paint.xform.get_inverse());
}

} // namespace notf
