#include "graphics/render_context_old.hpp"

#include "common/log.hpp"
#include "common/vector.hpp"
#include "core/glfw.hpp"
#include "core/opengl.hpp"
#include "core/window.hpp"
#include "graphics/gl_utils.hpp"
#include "graphics/texture2.hpp"

namespace { // anonymous

void xformToMat3x4(float* m3, notf::Xform2f t)
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

GLuint FRAG_BINDING = 0;

constexpr GLintptr fragmentSize()
{
    return sizeof(notf::RenderContext_Old::FragmentUniforms);
}

// clang-format off

// vertex shader source, read from file as described in: http://stackoverflow.com/a/25021520
const char* cell_vertex_shader =
#include "shader/cell.vert"

// fragment shader source
const char* cell_fragment_shader =
#include "shader/cell.frag"

// clang-format on
} // namespace anonymous

namespace notf {

using detail::CellPath;

RenderContext_Old* RenderContext_Old::s_current_context = nullptr;

RenderContext_Old::RenderContext_Old(const Window* window, const RenderContextArguments args)
    : m_window(window)
    , m_args(std::move(args))
    , m_buffer_size(Size2f(0, 0))
    , m_time()
    , m_stencil_mask(0xffffffff)
    , m_stencil_func(StencilFunc_Old::ALWAYS)
    , m_calls()
    , m_paths()
    , m_vertices()
    , m_frag_uniforms()
    , m_mouse_pos()
    , m_bound_texture(0)
    , m_textures()
    , m_bound_shader(0)
    , m_shaders()
    , m_sources(_create_shader_sources(*this))
    , m_cell_shader(build_shader("CellShader", m_sources.vertex, m_sources.fragment))
    , m_loc_viewsize(glGetUniformLocation(m_cell_shader->get_id(), "viewSize")) // TODO: these should be shader methods
    , m_loc_texture(glGetUniformLocation(m_cell_shader->get_id(), "tex"))
    , m_loc_buffer(glGetUniformBlockIndex(m_cell_shader->get_id(), "frag"))
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

    //  create dynamic vertex arrays
    glGenVertexArrays(1, &m_vertex_array);
    glGenBuffers(1, &m_vertex_buffer);

    // create UBOs
    glUniformBlockBinding(m_cell_shader->get_id(), m_loc_buffer, 0);
    glGenBuffers(1, &m_fragment_buffer);
    GLint align = sizeof(float);
    glGetIntegerv(GL_UNIFORM_BUFFER_OFFSET_ALIGNMENT, &align);

    glFinish();
}

RenderContext_Old::~RenderContext_Old()
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

void RenderContext_Old::make_current()
{
    if (s_current_context != this) {
        glfwMakeContextCurrent(m_window->_get_glfw_window());
        s_current_context = this;
    }
}

std::shared_ptr<Texture2> RenderContext_Old::load_texture(const std::string& file_path)
{
    std::shared_ptr<Texture2> texture = Texture2::load(this, file_path);
    if (texture) {
        m_textures.emplace_back(texture);
    }
    return texture;
}

std::shared_ptr<Shader> RenderContext_Old::build_shader(const std::string& name,
                                                    const std::string& vertex_shader_source,
                                                    const std::string& fragment_shader_source)
{
    std::shared_ptr<Shader> shader = Shader::build(this, name, vertex_shader_source, fragment_shader_source);
    if (shader) {
        m_shaders.emplace_back(shader);
    }
    return shader;
}

RenderContext_Old::FrameGuard RenderContext_Old::begin_frame(const Size2i buffer_size)
{
    m_calls.clear();
    m_paths.clear();
    m_vertices.clear();
    m_frag_uniforms.clear();

    m_buffer_size.width  = static_cast<float>(buffer_size.width);
    m_buffer_size.height = static_cast<float>(buffer_size.height);

    m_time = Time::now();

    return FrameGuard(this);
}

void RenderContext_Old::_abort_frame()
{
    m_calls.clear();
    m_paths.clear();
    m_vertices.clear();
    m_frag_uniforms.clear();
}

void RenderContext_Old::_end_frame()
{
    _render_flush(BlendMode::SOURCE_OVER);
}

void RenderContext_Old::add_fill_call(const Paint& paint, const Cell_Old& cell)
{
    m_calls.emplace_back();
    CanvasCall& call = m_calls.back();
    call.pathOffset  = m_paths.size();
    call.pathCount   = cell.get_paths().size();
    call.texture     = paint.texture;
    if (call.pathCount == 1 && cell.get_paths().front().is_convex) {
        call.type = CanvasCall::Type::CONVEX_FILL;
    }
    else {
        call.type = CanvasCall::Type::FILL;
    }

    //  this block is its own function in nanovg, called maxVertCount
    size_t new_vertex_count = 6; // + 6 for the quad that we construct further down
    for (const CellPath& path : cell.get_paths()) {
        new_vertex_count += path.fill_count;
        new_vertex_count += path.stroke_count;
    }
    //

    size_t offset = m_vertices.size();
    m_vertices.reserve(m_vertices.size() + new_vertex_count);
    m_paths.reserve(m_paths.size() + cell.get_paths().size());

    for (const CellPath& path : cell.get_paths()) {
        PathIndex path_index;
        if (path.fill_count != 0) {
            path_index.fillOffset = static_cast<GLint>(offset);
            path_index.fillCount  = static_cast<GLsizei>(path.fill_count);
            m_vertices.insert(std::end(m_vertices),
                              iterator_at(cell.get_vertices(), path.fill_offset),
                              iterator_at(cell.get_vertices(), path.fill_offset + path.fill_count));
            offset += path.fill_count;
        }
        if (path.stroke_count != 0) {
            path_index.strokeOffset = static_cast<GLint>(offset);
            path_index.strokeCount  = static_cast<GLsizei>(path.stroke_count);
            m_vertices.insert(std::end(m_vertices),
                              iterator_at(cell.get_vertices(), path.stroke_offset),
                              iterator_at(cell.get_vertices(), path.stroke_offset + path.stroke_count));
            offset += path.stroke_count;
        }
        m_paths.emplace_back(std::move(path_index));
    }

    // create a quad around the bounds of the filled area
    call.triangleOffset = static_cast<GLint>(offset);
    call.triangleCount  = 6;
    const Aabrf& bounds = cell.get_bounds();
    m_vertices.emplace_back(Vertex{Vector2f{bounds.left(), bounds.bottom()}, Vector2f{.5f, 1.f}});
    m_vertices.emplace_back(Vertex{Vector2f{bounds.right(), bounds.bottom()}, Vector2f{.5f, 1.f}});
    m_vertices.emplace_back(Vertex{Vector2f{bounds.right(), bounds.top()}, Vector2f{.5f, 1.f}});

    m_vertices.emplace_back(Vertex{Vector2f{bounds.left(), bounds.bottom()}, Vector2f{.5f, 1.f}});
    m_vertices.emplace_back(Vertex{Vector2f{bounds.right(), bounds.top()}, Vector2f{.5f, 1.f}});
    m_vertices.emplace_back(Vertex{Vector2f{bounds.left(), bounds.top()}, Vector2f{.5f, 1.f}});

    call.uniformOffset = static_cast<GLintptr>(m_frag_uniforms.size()) * fragmentSize();
    if (call.type == CanvasCall::Type::FILL) {
        // create an additional uniform buffer for a simple shader for the stencil
        m_frag_uniforms.emplace_back(FragmentUniforms());
        FragmentUniforms& stencil_uniforms = m_frag_uniforms.back();
        stencil_uniforms.strokeThr         = -1;
        stencil_uniforms.type              = FragmentUniforms::Type::SIMPLE;
    }

    m_frag_uniforms.push_back({});
    FragmentUniforms& fill_uniforms = m_frag_uniforms.back();
    _paint_to_frag(fill_uniforms, paint, cell.get_current_state().scissor,
                   cell.get_fringe_width(), cell.get_fringe_width(), -1.0f);
}

void RenderContext_Old::add_stroke_call(const Paint& paint, const float stroke_width, const Cell_Old& cell)
{
    m_calls.emplace_back();
    CanvasCall& call   = m_calls.back();
    call.type          = CanvasCall::Type::STROKE;
    call.pathOffset    = m_paths.size();
    call.pathCount     = cell.get_paths().size();
    call.uniformOffset = static_cast<GLintptr>(m_frag_uniforms.size()) * fragmentSize();
    call.texture       = paint.texture;

    size_t new_vertex_count = 0;
    for (const CellPath& path : cell.get_paths()) {
        new_vertex_count += path.fill_count;
        new_vertex_count += path.stroke_count;
    }

    size_t offset = m_vertices.size();
    m_vertices.reserve(m_vertices.size() + new_vertex_count);
    m_paths.reserve(m_paths.size() + cell.get_paths().size());

    for (const CellPath& path : cell.get_paths()) {
        PathIndex path_index;
        if (path.stroke_count != 0) {
            path_index.strokeOffset = static_cast<GLint>(offset);
            path_index.strokeCount  = static_cast<GLsizei>(path.stroke_count);
            m_vertices.insert(std::end(m_vertices),
                              iterator_at(cell.get_vertices(), path.stroke_offset),
                              iterator_at(cell.get_vertices(), path.stroke_offset + path.stroke_count));
            offset += path.stroke_count;
        }
        m_paths.emplace_back(std::move(path_index));
    }

    const Scissor_Old& scissor = cell.get_current_state().scissor;
    const float fringe     = cell.get_fringe_width();

    m_frag_uniforms.push_back({});
    FragmentUniforms& stencil_uniforms = m_frag_uniforms.back(); // I think that's what it is
    _paint_to_frag(stencil_uniforms, paint, scissor, stroke_width, fringe, -1.0f);

    m_frag_uniforms.push_back({});
    FragmentUniforms& stroke_uniforms = m_frag_uniforms.back();
    _paint_to_frag(stroke_uniforms, paint, scissor, stroke_width, fringe, 1.0f - 0.5f / 255.0f);
}

void RenderContext_Old::_paint_to_frag(FragmentUniforms& frag, const Paint& paint, const Scissor_Old& scissor,
                                   const float stroke_width, const float fringe, const float stroke_threshold)
{
    assert(fringe > 0);

    frag.innerCol = paint.inner_color.premultiplied();
    frag.outerCol = paint.outer_color.premultiplied();
    if (scissor.extend.width < 1.0f || scissor.extend.height < 1.0f) { // extend cannot be less than a pixel
        frag.scissorExt[0]   = 1.0f;
        frag.scissorExt[1]   = 1.0f;
        frag.scissorScale[0] = 1.0f;
        frag.scissorScale[1] = 1.0f;
    }
    else {
        xformToMat3x4(frag.scissorMat, scissor.xform.inverse());
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
        frag.type    = FragmentUniforms::Type::IMAGE;
        frag.texType = paint.texture->get_format() == Texture2::Format::GRAYSCALE ? 2 : 0; // TODO: change the 'texType' uniform in the shader to something more meaningful
    }
    else { // no image
        frag.type    = FragmentUniforms::Type::GRADIENT;
        frag.radius  = paint.radius;
        frag.feather = paint.feather;
    }
    xformToMat3x4(frag.paintMat, paint.xform.inverse());
}

void RenderContext_Old::_render_flush(const BlendMode blend_mode)
{
    if (!m_calls.empty()) {

        // reset cache
        m_stencil_mask  = 0xffffffff;
        m_bound_texture = 0;
        m_bound_shader  = 0;
        m_stencil_func  = StencilFunc_Old::ALWAYS;

        // setup GL state
        m_cell_shader->bind();
        blend_mode.apply();
        glEnable(GL_CULL_FACE);
        glCullFace(GL_BACK);
        glFrontFace(GL_CCW);
        glEnable(GL_BLEND);
        glDisable(GL_DEPTH_TEST);
        glDisable(GL_SCISSOR_TEST);
        glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
        glStencilMask(0xff);
        glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
        glStencilFunc(GL_ALWAYS, 0, 0xffffffff);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, 0);

        // Upload ubo for frag shaders
        glBindBuffer(GL_UNIFORM_BUFFER, m_fragment_buffer);
        glBufferData(GL_UNIFORM_BUFFER, static_cast<GLsizeiptr>(m_frag_uniforms.size()) * fragmentSize(), &m_frag_uniforms.front(), GL_STREAM_DRAW);

        // upload vertex data
        glBindVertexArray(m_vertex_array);
        glBindBuffer(GL_ARRAY_BUFFER, m_vertex_buffer);
        glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(m_vertices.size() * sizeof(Vertex)), &m_vertices.front(), GL_STREAM_DRAW);
        glEnableVertexAttribArray(0);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), nullptr);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), gl_buffer_offset(2 * sizeof(float)));

        // Set view and texture just once per frame.
        glUniform1i(m_loc_texture, 0);
        glUniform2fv(m_loc_viewsize, 1, m_buffer_size.as_ptr());

        // perform the render calls
        for (const CanvasCall& call : m_calls) {
            switch (call.type) {
            case CanvasCall::Type::FILL:
                _fill(call);
                break;
            case CanvasCall::Type::CONVEX_FILL:
                _convex_fill(call);
                break;
            case CanvasCall::Type::STROKE:
                _stroke(call);
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

    // reset layer state
    m_vertices.clear();
    m_paths.clear();
    m_calls.clear();
    m_frag_uniforms.clear();
}

void RenderContext_Old::_fill(const CanvasCall& call)
{
    assert(call.pathOffset + call.pathCount <= m_paths.size());
    assert(static_cast<size_t>(call.uniformOffset) <= max(size_t(0), m_frag_uniforms.size() - 1) * fragmentSize());

    // Draw shapes
    glEnable(GL_STENCIL_TEST);
    set_stencil_mask(0xff);
    set_stencil_func(StencilFunc_Old::ALWAYS);
    glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);

    // set bindpoint for solid loc
    glBindBufferRange(GL_UNIFORM_BUFFER, FRAG_BINDING, m_fragment_buffer, call.uniformOffset, fragmentSize());

    glStencilOpSeparate(GL_FRONT, GL_KEEP, GL_KEEP, GL_INCR_WRAP);
    glStencilOpSeparate(GL_BACK, GL_KEEP, GL_KEEP, GL_DECR_WRAP);
    glDisable(GL_CULL_FACE);
    for (size_t i = call.pathOffset; i < call.pathOffset + call.pathCount; ++i) {
        assert(static_cast<size_t>(m_paths[i].fillOffset + m_paths[i].fillCount) <= m_vertices.size());
        glDrawArrays(GL_TRIANGLE_FAN, m_paths[i].fillOffset, m_paths[i].fillCount);
    }
    glEnable(GL_CULL_FACE);

    // Draw anti-aliased pixels
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    glBindBufferRange(GL_UNIFORM_BUFFER, FRAG_BINDING, m_fragment_buffer, call.uniformOffset + fragmentSize(), fragmentSize());
    if (call.texture) {
        _bind_texture(call.texture->get_id());
    }

    if (m_args.enable_geometric_aa) {
        set_stencil_func(StencilFunc_Old::EQUAL);
        glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
        // Draw fringes
        for (size_t i = call.pathOffset; i < call.pathOffset + call.pathCount; ++i) {
            assert(static_cast<size_t>(m_paths[i].strokeOffset + m_paths[i].strokeCount) <= m_vertices.size());
            glDrawArrays(GL_TRIANGLE_STRIP, m_paths[i].strokeOffset, m_paths[i].strokeCount);
        }
    }

    // Draw fill
    set_stencil_func(StencilFunc_Old::NOTEQUAL);
    glStencilOp(GL_ZERO, GL_ZERO, GL_ZERO);
    glDrawArrays(GL_TRIANGLES, call.triangleOffset, call.triangleCount);

    glDisable(GL_STENCIL_TEST);
}

void RenderContext_Old::_convex_fill(const CanvasCall& call)
{
    assert(call.pathOffset + call.pathCount <= m_paths.size());

    glBindBufferRange(GL_UNIFORM_BUFFER, FRAG_BINDING, m_fragment_buffer, call.uniformOffset, fragmentSize());
    if (call.texture) {
        _bind_texture(call.texture->get_id());
    }

    for (size_t i = call.pathOffset; i < call.pathOffset + call.pathCount; ++i) {
        // draw fill
        assert(static_cast<size_t>(m_paths[i].fillOffset + m_paths[i].fillCount) <= m_vertices.size());
        glDrawArrays(GL_TRIANGLE_FAN, m_paths[i].fillOffset, m_paths[i].fillCount);
    }
    if (m_args.enable_geometric_aa) {
        // draw fringes
        for (size_t i = call.pathOffset; i < call.pathOffset + call.pathCount; ++i) {
            assert(static_cast<size_t>(m_paths[i].strokeOffset + m_paths[i].strokeCount) <= m_vertices.size());
            glDrawArrays(GL_TRIANGLE_STRIP, m_paths[i].strokeOffset, m_paths[i].strokeCount);
        }
    }
}

void RenderContext_Old::_stroke(const CanvasCall& call)
{
    assert(call.pathOffset + call.pathCount <= m_paths.size());
    assert(static_cast<size_t>(call.uniformOffset) <= max(size_t(0), m_frag_uniforms.size() - 1) * fragmentSize());

    glEnable(GL_STENCIL_TEST);
    set_stencil_mask(0xff);

    // fill the stroke base without overlap
    set_stencil_func(StencilFunc_Old::EQUAL);
    glStencilOp(GL_KEEP, GL_KEEP, GL_INCR);
    glBindBufferRange(GL_UNIFORM_BUFFER, FRAG_BINDING, m_fragment_buffer, call.uniformOffset + fragmentSize(), fragmentSize());
    if (call.texture) {
        _bind_texture(call.texture->get_id());
    }
    for (size_t i = call.pathOffset; i < call.pathOffset + call.pathCount; ++i) {
        assert(static_cast<size_t>(m_paths[i].strokeOffset + m_paths[i].strokeCount) <= m_vertices.size());
        glDrawArrays(GL_TRIANGLE_STRIP, m_paths[i].strokeOffset, m_paths[i].strokeCount);
    }

    // draw anti-aliased pixels
    glBindBufferRange(GL_UNIFORM_BUFFER, FRAG_BINDING, m_fragment_buffer, call.uniformOffset, fragmentSize());
    if (call.texture) {
        _bind_texture(call.texture->get_id());
    }
    set_stencil_func(StencilFunc_Old::EQUAL);
    glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
    for (size_t i = call.pathOffset; i < call.pathOffset + call.pathCount; ++i) {
        assert(static_cast<size_t>(m_paths[i].strokeOffset + m_paths[i].strokeCount) <= m_vertices.size());
        glDrawArrays(GL_TRIANGLE_STRIP, m_paths[i].strokeOffset, m_paths[i].strokeCount);
    }

    // clear stencil buffer
    glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
    set_stencil_func(StencilFunc_Old::ALWAYS);
    glStencilOp(GL_ZERO, GL_ZERO, GL_ZERO);
    for (size_t i = call.pathOffset; i < call.pathOffset + call.pathCount; ++i) {
        assert(static_cast<size_t>(m_paths[i].strokeOffset + m_paths[i].strokeCount) <= m_vertices.size());
        glDrawArrays(GL_TRIANGLE_STRIP, m_paths[i].strokeOffset, m_paths[i].strokeCount);
    }
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);

    glDisable(GL_STENCIL_TEST);
}

void RenderContext_Old::_bind_texture(const GLuint texture_id)
{
    if (texture_id != m_bound_texture) {
        glBindTexture(GL_TEXTURE_2D, texture_id);
        m_bound_texture = texture_id;
    }
}

void RenderContext_Old::_bind_shader(const GLuint shader_id)
{
    if (shader_id != m_bound_shader) {
        glUseProgram(shader_id);
        m_bound_shader = shader_id;
    }
}

void RenderContext_Old::set_stencil_mask(const GLuint mask)
{
    if (mask != m_stencil_mask) {
        m_stencil_mask = mask;
        glStencilMask(mask);
    }
}

void RenderContext_Old::set_stencil_func(const StencilFunc_Old func)
{
    if (func == m_stencil_func) {
        return;
    }
    m_stencil_func = func;
    switch (m_stencil_func) {
    case StencilFunc_Old::ALWAYS:
        glStencilFunc(GL_ALWAYS, 0x00, 0xff);
        break;
    case StencilFunc_Old::NEVER:
        glStencilFunc(GL_NEVER, 0x00, 0xff);
        break;
    case StencilFunc_Old::LESS:
        glStencilFunc(GL_LESS, 0x00, 0xff);
        break;
    case StencilFunc_Old::LEQUAL:
        glStencilFunc(GL_LEQUAL, 0x00, 0xff);
        break;
    case StencilFunc_Old::GREATER:
        glStencilFunc(GL_GREATER, 0x00, 0xff);
        break;
    case StencilFunc_Old::GEQUAL:
        glStencilFunc(GL_GEQUAL, 0x00, 0xff);
        break;
    case StencilFunc_Old::EQUAL:
        glStencilFunc(GL_EQUAL, 0x00, 0xff);
        break;
    case StencilFunc_Old::NOTEQUAL:
        glStencilFunc(GL_NOTEQUAL, 0x00, 0xff);
        break;
    }
}

RenderContext_Old::Sources RenderContext_Old::_create_shader_sources(const RenderContext_Old& context)
{
    // create the header
    std::string header;
    header += "#version 300 es\n";
    header += "#define UNIFORMARRAY_SIZE 11\n";
    if (context.provides_geometric_aa()) {
        header += "#define GEOMETRY_AA 1\n";
    }
    header += "\n";

    // TODO: building the shader is wasteful (but it only happens once...)

    // attach the header to the source files
    return {header + cell_vertex_shader,
            header + cell_fragment_shader};
}

} // namespace notf