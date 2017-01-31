#include "graphics2/hud_layer.hpp"

#include "common/log.hpp"
#include "common/vector_utils.hpp"
#include "core/glfw_wrapper.hpp"
#include "graphics2/backend.hpp"
#include "graphics2/hud_shader.hpp"

namespace { // anonymous

void xformToMat3x4(float* m3, notf::Transform2 t)
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
    m3[9]  = t[2][2];
    m3[10] = 1.0f;
    m3[11] = 0.0f;
}

GLuint FRAG_BINDING = 0;

} // namespace anonymous

namespace notf {

HUDShader HUDLayer::s_shader = {}; // invalid by default

HUDLayer::HUDLayer(const RenderBackend& backend, const float pixel_ratio)
    : m_backend(backend)
    , m_viewport_size(Size2f(0, 0))
    , m_pixel_ratio(pixel_ratio)
    , m_stencil_mask(0xffffffff)
    , m_stencil_func(StencilFunc::ALWAYS)
    , m_calls()
    , m_paths()
    , m_vertices()
    , m_frag_uniforms()
{
    if (!s_shader.is_valid()) {
        s_shader = HUDShader(backend);
        if (!s_shader.is_valid()) {
            log_critical << "Failed to construct the HUD shader!";
        }
    }

    set_pixel_ratio(m_pixel_ratio);
}

void HUDLayer::begin_frame(const int width, const int height) // called from "beginFrame"
{
    m_calls.clear();
    m_paths.clear();

    m_viewport_size.width  = static_cast<float>(width);
    m_viewport_size.height = static_cast<float>(height);
}

void HUDLayer::abort_frame()
{
    m_calls.clear();
    m_paths.clear();
    //  gl->nverts = 0;
    //	gl->nuniforms = 0;
}

void HUDLayer::end_frame()
{
}

void HUDLayer::add_fill_call(const Paint& paint, const Scissor& scissor, float fringe, const Aabr& bounds,
                             const std::vector<Path>& paths)
{
    m_calls.emplace_back();
    HUDCall& call   = m_calls.back();
    call.pathOffset = m_paths.size();
    call.pathCount  = paths.size();
    if (paths.size() == 1 && paths[0].is_convex) {
        call.type = HUDCall::Type::CONVEX_FILL;
    }
    else {
        call.type = HUDCall::Type::FILL;
    }

    //  this block is its own function in nanovg, called maxVertCount
    size_t new_vertex_count = 6; // + 6 for the quad that we construct further down
    for (const Path& path : paths) {
        new_vertex_count += path.fill.size();
        new_vertex_count += path.stroke.size();
    }
    //

    size_t offset = m_vertices.size();
    m_vertices.reserve(m_vertices.size() + new_vertex_count);
    m_paths.reserve(m_paths.size() + paths.size());

    for (const Path& path : paths) {
        HUDPath hud_path;
        if (!path.fill.empty()) {
            hud_path.fillOffset = offset;
            hud_path.fillCount  = path.fill.size();
            append(m_vertices, path.fill);
            offset += path.fill.size();
        }
        if (!path.stroke.empty()) {
            hud_path.strokeOffset = offset;
            hud_path.strokeCount  = path.stroke.size();
            append(m_vertices, path.stroke);
            offset += path.stroke.size();
        }
        m_paths.emplace_back(std::move(hud_path));
    }

    // create a quad around the bounds of the filled area
    assert(offset == m_vertices.size() - 6);
    call.triangleOffset = offset;
    call.triangleCount  = 6;
    m_vertices.emplace_back(bounds.left(), bounds.bottom(), .5f, 1.f);
    m_vertices.emplace_back(bounds.right(), bounds.bottom(), .5f, 1.f);
    m_vertices.emplace_back(bounds.right(), bounds.top(), .5f, 1.f);

    m_vertices.emplace_back(bounds.left(), bounds.bottom(), .5f, 1.f);
    m_vertices.emplace_back(bounds.right(), bounds.top(), .5f, 1.f);
    m_vertices.emplace_back(bounds.left(), bounds.top(), .5f, 1.f);

    call.uniformOffset = static_cast<GLintptr>(m_frag_uniforms.size());
    if (call.type == HUDCall::Type::FILL) {
        // create an additional uniform buffer for a simple shader for the stencil
        m_frag_uniforms.push_back({});
        FragmentUniforms& stencil_uniforms = m_frag_uniforms.back();
        stencil_uniforms.strokeThr         = -1;
        stencil_uniforms.type              = FragmentUniforms::Type::SIMPLE;
    }

    m_frag_uniforms.push_back({});
    FragmentUniforms& fill_uniforms = m_frag_uniforms.back();
    _paint_to_frag(fill_uniforms, paint, scissor, fringe, fringe, -1.0f);
}

void HUDLayer::add_stroke_call(const Paint& paint, const Scissor& scissor, float fringe, float strokeWidth,
                               const std::vector<Path>& paths)
{
    m_calls.emplace_back();
    HUDCall& call   = m_calls.back();
    call.type       = HUDCall::Type::STROKE;
    call.pathOffset = m_paths.size();
    call.pathCount  = paths.size();

    size_t new_vertex_count = 0;
    for (const Path& path : paths) {
        new_vertex_count += path.fill.size();
        new_vertex_count += path.stroke.size();
    }

    size_t offset = m_vertices.size();
    m_vertices.reserve(m_vertices.size() + new_vertex_count);
    m_paths.reserve(m_paths.size() + paths.size());

    for (const Path& path : paths) {
        HUDPath hud_path;
        if (!path.stroke.empty()) {
            hud_path.strokeOffset = offset;
            hud_path.strokeCount  = path.stroke.size();
            append(m_vertices, path.stroke);
            offset += path.stroke.size();
        }
    }

    m_frag_uniforms.push_back({});
    FragmentUniforms& stencil_uniforms = m_frag_uniforms.back(); // I think that's what it is
    _paint_to_frag(stencil_uniforms, paint, scissor, strokeWidth, fringe, -1.0f);

    m_frag_uniforms.push_back({});
    FragmentUniforms& stroke_uniforms = m_frag_uniforms.back();
    _paint_to_frag(stroke_uniforms, paint, scissor, strokeWidth, fringe, 1.0f - 0.5f / 255.0f);

    // TODO: nanovg checks for allocation errors while I just resize the vectors .. is that reasonable?
}

void HUDLayer::_paint_to_frag(FragmentUniforms& frag, const Paint& paint, const Scissor& scissor,
                              const float stroke_width, const float fringe, const float stroke_threshold)
{
    assert(fringe > 0);

    frag.innerCol = paint.innerColor.premultiplied();
    frag.outerCol = paint.outerColor.premultiplied();
    if (scissor.extend.width < -0.5f || scissor.extend.height < -0.5f) { // what's with the magic -0.5 here?
        frag.scissorExt[0]   = 1.0f;
        frag.scissorExt[1]   = 1.0f;
        frag.scissorScale[0] = 1.0f;
        frag.scissorScale[1] = 1.0f;
    }
    else {
        xformToMat3x4(frag.scissorMat, scissor.xform.inverse());
        frag.scissorExt[0]   = scissor.extend.width;
        frag.scissorExt[1]   = scissor.extend.height;
        frag.scissorScale[0] = sqrt(scissor.xform[0][0] * scissor.xform[0][0] + scissor.xform[1][0] * scissor.xform[1][0]) / fringe;
        frag.scissorScale[1] = sqrt(scissor.xform[0][1] * scissor.xform[0][1] + scissor.xform[1][1] * scissor.xform[1][1]) / fringe;
    }
    frag.extent[0]  = paint.extent.width;
    frag.extent[1]  = paint.extent.height;
    frag.strokeMult = (stroke_width * 0.5f + fringe * 0.5f) / fringe;
    frag.strokeThr  = stroke_threshold;
    frag.type       = FragmentUniforms::Type::GRADIENT;
    frag.radius     = paint.radius;
    frag.feather    = paint.feather;
    xformToMat3x4(frag.paintMat, paint.xform.inverse());
}

void HUDLayer::_render_flush(const BlendMode blend_mode)
{
    if (!m_calls.empty()) {

        // reset cache
        m_stencil_mask = 0xffffffff;
        m_stencil_func = StencilFunc::ALWAYS;

        // setup GL state
        glUseProgram(s_shader.get_id());
        BlendMode::Arguments blendArgs = blend_mode.get_arguments();
        glBlendFuncSeparate(blendArgs.rgb_sfactor, blendArgs.rgb_dfactor, blendArgs.alpha_sfactor, blendArgs.alpha_dfactor);
        glEnable(GL_CULL_FACE);
        glCullFace(GL_BACK);
        glFrontFace(GL_CCW);
        glEnable(GL_BLEND);
        glDisable(GL_DEPTH_TEST);
        glDisable(GL_SCISSOR_TEST);
        glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
        glStencilMask(0xffffffff);
        glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
        glStencilFunc(GL_ALWAYS, 0, 0xffffffff);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, 0);

        // Upload ubo for frag shaders
        glBindBuffer(GL_UNIFORM_BUFFER, s_shader.m_fragment_buffer); // this is where the shader / layer separation breaks apart
        glBufferData(GL_UNIFORM_BUFFER, static_cast<GLsizeiptr>(m_frag_uniforms.size()) * fragSize(), &m_frag_uniforms.front(), GL_STREAM_DRAW);

        // upload vertex data
        glBindVertexArray(s_shader.m_vertex_array);
        glBindBuffer(GL_ARRAY_BUFFER, s_shader.m_vertex_buffer);
        glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(m_vertices.size() * sizeof(Vertex)), &m_vertices.front(), GL_STREAM_DRAW);
        glEnableVertexAttribArray(0);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), nullptr);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (const GLvoid*)(2 * sizeof(float)));

        // Set view and texture just once per frame.
        glUniform1i(s_shader.m_loc_texture, 0);
        glUniform2fv(s_shader.m_loc_viewsize, 1, m_viewport_size.as_float_ptr());
        glBindBuffer(GL_UNIFORM_BUFFER, s_shader.m_fragment_buffer);

        // perform the render calls
        for (const HUDCall& call : m_calls) {
            switch (call.type) {
            case HUDCall::Type::FILL:
                _fill(call);
                break;
            case HUDCall::Type::CONVEX_FILL:
                _convex_fill(call);
                break;
            case HUDCall::Type::STROKE:
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
        glUseProgram(0);
    }

    // reset layer state
    m_vertices.clear();
    m_paths.clear();
    m_calls.clear();
    m_frag_uniforms.clear();
}

void HUDLayer::_fill(const HUDCall& call)
{
    // Draw shapes
    glEnable(GL_STENCIL_TEST);
    set_stencil_mask(0xff);
    set_stencil_func(StencilFunc::ALWAYS);
    glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);

    // set bindpoint for solid loc
    glBindBufferRange(GL_UNIFORM_BUFFER, FRAG_BINDING, s_shader.m_fragment_buffer, call.uniformOffset, sizeof(FragmentUniforms));

    glStencilOpSeparate(GL_FRONT, GL_KEEP, GL_KEEP, GL_INCR_WRAP);
    glStencilOpSeparate(GL_BACK, GL_KEEP, GL_KEEP, GL_DECR_WRAP);
    glDisable(GL_CULL_FACE);
    for (size_t i = call.pathOffset; i < call.pathOffset + call.pathCount; ++i){
        glDrawArrays(GL_TRIANGLE_FAN, m_paths[i].fillOffset, m_paths[i].fillCount);
    }
    glEnable(GL_CULL_FACE);

    // Draw anti-aliased pixels
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    glBindBufferRange(GL_UNIFORM_BUFFER, FRAG_BINDING, s_shader.m_fragment_buffer, call.uniformOffset + fragSize(), sizeof(FragmentUniforms));

    if (m_backend.has_msaa) {
        set_stencil_func(StencilFunc::EQUAL);
        glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
        // Draw fringes
        for (size_t i = call.pathOffset; i < call.pathOffset + call.pathCount; ++i){
            glDrawArrays(GL_TRIANGLE_STRIP, m_paths[i].strokeOffset, m_paths[i].strokeCount);
        }
    }

    // Draw fill
    set_stencil_func(StencilFunc::NOTEQUAL);
    glStencilOp(GL_ZERO, GL_ZERO, GL_ZERO);
    glDrawArrays(GL_TRIANGLES, call.triangleOffset, call.triangleCount);

    glDisable(GL_STENCIL_TEST);
}

void HUDLayer::_convex_fill(const HUDCall& call)
{
    glBindBufferRange(GL_UNIFORM_BUFFER, 0, s_shader.m_fragment_buffer, call.uniformOffset, sizeof(FragmentUniforms));

    for (size_t i = call.pathOffset; i < call.pathOffset + call.pathCount; ++i) {
        // draw fill
        glDrawArrays(GL_TRIANGLE_FAN, m_paths[i].fillOffset, m_paths[i].fillCount);
    }
    if (m_backend.has_msaa) {
        // draw fringes
        for (size_t i = call.pathOffset; i < call.pathOffset + call.pathCount; ++i) {
            glDrawArrays(GL_TRIANGLE_STRIP, m_paths[i].strokeOffset, m_paths[i].strokeCount);
        }
    }
}

void HUDLayer::_stroke(const HUDCall& call)
{
    glEnable(GL_STENCIL_TEST);
    set_stencil_mask(0xff);

    // Fill the stroke base without overlap
    set_stencil_func(StencilFunc::EQUAL);
    glStencilOp(GL_KEEP, GL_KEEP, GL_INCR);
    glBindBufferRange(GL_UNIFORM_BUFFER, FRAG_BINDING, s_shader.m_fragment_buffer,
                      call.uniformOffset + fragSize(), sizeof(FragmentUniforms));
    for (size_t i = call.pathOffset; i < call.pathOffset + call.pathCount; ++i) {
        glDrawArrays(GL_TRIANGLE_STRIP, m_paths[i].strokeOffset, m_paths[i].strokeCount);
    }

    // Draw anti-aliased pixels.
    glBindBufferRange(GL_UNIFORM_BUFFER, FRAG_BINDING, s_shader.m_fragment_buffer, call.uniformOffset, sizeof(FragmentUniforms));
    set_stencil_func(StencilFunc::EQUAL);
    glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
    for (size_t i = call.pathOffset; i < call.pathOffset + call.pathCount; ++i) {
        glDrawArrays(GL_TRIANGLE_STRIP, m_paths[i].strokeOffset, m_paths[i].strokeCount);
    }

    // Clear stencil buffer.
    glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
    set_stencil_func(StencilFunc::ALWAYS);
    glStencilOp(GL_ZERO, GL_ZERO, GL_ZERO);
    for (size_t i = call.pathOffset; i < call.pathOffset + call.pathCount; ++i) {
        glDrawArrays(GL_TRIANGLE_STRIP, m_paths[i].strokeOffset, m_paths[i].strokeCount);
    }
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);

    glDisable(GL_STENCIL_TEST);
}

void HUDLayer::set_stencil_mask(const GLuint mask)
{
    if (mask != m_stencil_mask) {
        m_stencil_mask = mask;
        glStencilMask(mask);
    }
}

void HUDLayer::set_stencil_func(const StencilFunc func)
{
    if (func == m_stencil_func) {
        return;
    }
    switch (func) {
    case StencilFunc::ALWAYS:
        glStencilFunc(GL_ALWAYS, 0x00, 0xff);
        break;
    case StencilFunc::NEVER:
        glStencilFunc(GL_NEVER, 0x00, 0xff);
        break;
    case StencilFunc::LESS:
        glStencilFunc(GL_LESS, 0x00, 0xff);
        break;
    case StencilFunc::LEQUAL:
        glStencilFunc(GL_LEQUAL, 0x00, 0xff);
        break;
    case StencilFunc::GREATER:
        glStencilFunc(GL_GREATER, 0x00, 0xff);
        break;
    case StencilFunc::GEQUAL:
        glStencilFunc(GL_GEQUAL, 0x00, 0xff);
        break;
    case StencilFunc::EQUAL:
        glStencilFunc(GL_EQUAL, 0x00, 0xff);
        break;
    case StencilFunc::NOTEQUAL:
        glStencilFunc(GL_NOTEQUAL, 0x00, 0xff);
        break;
    }
}

} // namespace notf
