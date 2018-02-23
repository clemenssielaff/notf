#include "graphics/core/frame_buffer.hpp"

#include <algorithm>
#include <cassert>
#include <set>
#include <sstream>

#include "common/exception.hpp"
#include "common/log.hpp"
#include "graphics/core/gl_errors.hpp"
#include "graphics/core/graphics_context.hpp"
#include "graphics/core/opengl.hpp"
#include "graphics/core/texture.hpp"
#include "utils/make_smart_enabler.hpp"

namespace { //
using namespace notf;

/// Human-readable name of a RenderBuffer type.
const char* type_to_str(const RenderBuffer::Type type)
{
    switch (type) {
    case RenderBuffer::Type::INVALID:
        return "INVALID";
    case RenderBuffer::Type::COLOR:
        return "COLOR";
    case RenderBuffer::Type::DEPTH:
        return "DEPTH";
    case RenderBuffer::Type::STENCIL:
        return "STENCIL";
    case RenderBuffer::Type::DEPTH_STENCIL:
        return "DEPTH_STENCIL";
    }
}

/// Humand-readable error string for status returned by glCheckFramebufferStatus.
const char* status_to_str(const GLenum status)
{
    switch (status) {
    case (GL_FRAMEBUFFER_UNDEFINED):
        return "GL_FRAMEBUFFER_UNDEFINED";
    case (GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT):
        return "GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT";
    case (GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT):
        return "GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT";
    case (GL_FRAMEBUFFER_UNSUPPORTED):
        return "GL_FRAMEBUFFER_UNSUPPORTED";
    case (GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE):
        return "GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE";
    default:
        assert(0);
    }
    return nullptr;
}

} // namespace

//====================================================================================================================//

namespace notf {

RenderBuffer::RenderBuffer(GraphicsContextPtr& context, Args&& args)
    : m_id(0), m_graphics_context(*context), m_args(std::move(args))
{
    // check arguments
    const GraphicsContext::Environment& env = m_graphics_context.environment();
    if (!m_args.size.is_valid() || m_args.size.area() == 0
        || m_args.size.height > static_cast<short>(env.max_render_buffer_size)
        || m_args.size.width > static_cast<short>(env.max_render_buffer_size)) {
        notf_throw_format(runtime_error, "Invalid render buffer size: " << args.size);
    }
    if (m_args.internal_format == 0) {
        notf_throw(runtime_error, "Invalid internal format for RenderBuffer: 0");
    }
    if (m_args.type == Type::COLOR) {
        _assert_color_format(m_args.internal_format);
    }
    else {
        _assert_depth_stencil_format(m_args.internal_format);
    }

    { // generate new renderbuffer id
        RenderBufferId::underlying_t id = 0;
        gl_check(glGenRenderbuffers(1, &id));
        m_id = id;
        if (m_id == 0) {
            notf_throw(runtime_error, "Could not allocate new RenderBuffer");
        }
    }
}

RenderBufferPtr RenderBuffer::create(GraphicsContextPtr& context, Args&& args)
{
    RenderBufferPtr result;
#ifdef NOTF_DEBUG
    result = RenderBufferPtr(new RenderBuffer(context, std::move(args)));
#else
    result = std::make_shared<make_shared_enabler<RenderBuffer>>(context, std::move(args));
#endif
    return result;
}

RenderBuffer::~RenderBuffer() { _deallocate(); }

void RenderBuffer::_assert_color_format(const GLenum internal_format)
{
    switch (internal_format) {
    case GL_R8:
    case GL_R8I:
    case GL_R8UI:
    case GL_R16I:
    case GL_R16UI:
    case GL_R32I:
    case GL_R32UI:
    case GL_RG8:
    case GL_RG8I:
    case GL_RG8UI:
    case GL_RG16I:
    case GL_RG16UI:
    case GL_RG32I:
    case GL_RG32UI:
    case GL_RGB8:
    case GL_RGB565:
    case GL_RGBA8:
    case GL_SRGB8_ALPHA8:
    case GL_RGB5_A1:
    case GL_RGBA4:
    case GL_RGB10_A2:
    case GL_RGBA8I:
    case GL_RGBA8UI:
    case GL_RGB10_A2UI:
    case GL_RGBA16I:
    case GL_RGBA16UI:
    case GL_RGBA32I:
    case GL_RGBA32UI:
        break;
    default: {
        notf_throw_format(runtime_error, "Invalid internal format for a color buffer: " << internal_format);
    }
    }
}

void RenderBuffer::_assert_depth_stencil_format(const GLenum internal_format)
{
    switch (internal_format) {
    case GL_DEPTH_COMPONENT16:
    case GL_DEPTH_COMPONENT24:
    case GL_DEPTH_COMPONENT32F:
    case GL_DEPTH24_STENCIL8:
    case GL_DEPTH32F_STENCIL8:
    case GL_STENCIL_INDEX8:
        break;
    default: {
        notf_throw_format(runtime_error, "Invalid internal format for a depth or stencil buffer: " << internal_format);
    }
    }
}

void RenderBuffer::_deallocate()
{
    if (m_id != 0) {
        gl_check(glDeleteRenderbuffers(1, &m_id.value()));
        m_id = RenderBufferId::invalid();
    }
}

//====================================================================================================================//

void FrameBuffer::Args::set_color_target(const ushort id, ColorTarget target)
{
    auto it = std::find_if(color_targets.begin(), color_targets.end(),
                           [id](const std::pair<ushort, ColorTarget>& target) -> bool { return target.first == id; });
    if (it == color_targets.end()) {
        color_targets.emplace_back(std::make_pair(id, std::move(target)));
    }
    else {
        it->second = std::move(target);
    }
}

FrameBuffer::FrameBuffer(GraphicsContext& context, Args&& args) : m_graphics_context(context), m_id(0), m_args(args)
{
    _validate_arguments();

    { // generate new framebuffer id
        FrameBufferId::underlying_t id = 0;
        gl_check(glGenFramebuffers(1, &id));
        m_id = id;
        if (m_id == 0) {
            notf_throw(runtime_error, "Could not allocate new FrameBuffer");
        }
    }

    // define framebuffer
    gl_check(glBindFramebuffer(GL_FRAMEBUFFER, m_id.value()));

    for (const auto& numbered_color_target : m_args.color_targets) {
        const ushort target_id   = numbered_color_target.first;
        const auto& color_target = numbered_color_target.second;

        if (std::holds_alternative<RenderBufferPtr>(color_target)) {
            if (const auto& color_buffer = std::get<RenderBufferPtr>(color_target)) {
                gl_check(glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + target_id, GL_RENDERBUFFER,
                                                   color_buffer->id().value()));
            }
        }
        else if (const auto& color_texture = std::get<TexturePtr>(color_target)) {
            gl_check(glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + target_id, color_texture->target(),
                                            color_texture->id().value(), /* level = */ 0));
        }
    }

    bool has_depth = false;
    if (std::holds_alternative<RenderBufferPtr>(m_args.depth_target)) {
        if (const auto& depth_buffer = std::get<RenderBufferPtr>(m_args.depth_target)) {
            gl_check(glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER,
                                               depth_buffer->id().value()));
            has_depth = true;
        }
    }
    else if (const auto& depth_texture = std::get<TexturePtr>(m_args.depth_target)) {
        gl_check(glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, depth_texture->target(),
                                        depth_texture->id().value(), /* level = */ 0));
        has_depth = true;
    }

    if (m_args.stencil_target) {
        gl_check(glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_STENCIL_ATTACHMENT, GL_RENDERBUFFER,
                                           m_args.stencil_target->id().value()));
    }

    { // make sure the frame buffer is valid
        GLenum status          = glCheckFramebufferStatus(GL_FRAMEBUFFER);
        const bool has_stencil = (m_args.stencil_target == nullptr);
        if (status == GL_FRAMEBUFFER_COMPLETE) {
            log_info << "Created new FrameBuffer " << m_id << "with " << m_args.color_targets.size()
                     << " color attachments" << (has_depth ? (has_depth && has_stencil ? "," : " and") : "")
                     << (has_depth ? " a depth attachment" : "") << (has_stencil ? " and a stencil attachment" : "");
        }
        else {
            notf_throw_format(runtime_error, "OpenGL error: " << status_to_str(status));
        }
    }

    gl_check(glBindFramebuffer(GL_FRAMEBUFFER, 0));
}

FrameBufferPtr FrameBuffer::create(GraphicsContext& context, Args&& args)
{
    FrameBufferPtr framebuffer;
#ifdef NOTF_DEBUG
    framebuffer = FrameBufferPtr(new FrameBuffer(context, std::move(args)));
#else
    framebuffer = std::make_shared<make_shared_enabler<FrameBuffer>>(context, std::move(args));
#endif
    GraphicsContext::Private<FrameBuffer>(context).register_new(framebuffer);
    return framebuffer;
}

FrameBuffer::~FrameBuffer() { _deallocate(); }

const TexturePtr& FrameBuffer::color_texture(const ushort id)
{
    auto itr = std::find_if(std::begin(m_args.color_targets), std::end(m_args.color_targets),
                            [id](const std::pair<ushort, ColorTarget>& target) -> bool { return target.first == id; });
    try {
        if (itr != std::end(m_args.color_targets)) {
            return std::get<TexturePtr>(itr->second);
        }
    }
    catch (std::bad_variant_access&) {
        /* ignore */
    }
    notf_throw_format(runtime_error, "FrameBuffer has no color attachment: " << id);
}

void FrameBuffer::_deallocate()
{
    if (m_id == 0) {
        return;
    }

    // deallocate all attached RenderBuffers
    for (const auto& numbered_color_target : m_args.color_targets) {
        const auto& color_target = numbered_color_target.second;
        if (std::holds_alternative<RenderBufferPtr>(color_target)) {
            assert(std::get<RenderBufferPtr>(color_target)); // RenderBuffer pointer should never be empty
            std::get<RenderBufferPtr>(color_target)->_deallocate();
        }
    }

    if (std::holds_alternative<RenderBufferPtr>(m_args.depth_target)) {
        assert(std::get<RenderBufferPtr>(m_args.depth_target)); // RenderBuffer pointer should never be empty
        std::get<RenderBufferPtr>(m_args.depth_target)->_deallocate();
    }

    if (m_args.stencil_target) {
        m_args.stencil_target->_deallocate();
    }

    // shed all owning pointers
    m_args = Args{};

    // deallocate yourself
    gl_check(glDeleteFramebuffers(1, &m_id.value()));
    m_id = FrameBufferId::invalid();
}

/// For the rules, check:
/// https://www.khronos.org/opengl/wiki/Framebuffer_Object#Framebuffer_Completeness
void FrameBuffer::_validate_arguments() const
{
    bool has_any_attachment = false;             // is there at least one valid attachment for the frame buffer?
    std::set<ushort> used_targets;               // is any color target used twice?
    Size2s framebuffer_size = Size2s::invalid(); // do all framebuffers have the same size?

    // color attachments
    for (const auto& numbered_color_target : m_args.color_targets) {
        const ushort target_id = numbered_color_target.first;
        if (used_targets.count(target_id)) {
            notf_throw_format(runtime_error, "Duplicate color attachment id: " << target_id);
        }
        used_targets.insert(target_id);

        const auto& color_target = numbered_color_target.second;
        if (std::holds_alternative<RenderBufferPtr>(color_target)) {
            if (const auto& color_buffer = std::get<RenderBufferPtr>(color_target)) {
                if (color_buffer->type() != RenderBuffer::Type::COLOR) {
                    notf_throw_format(runtime_error, "Cannot attach a RenderBuffer of type "
                                                         << type_to_str(color_buffer->type()) << " as color buffer");
                }

                if (framebuffer_size.is_valid()) {
                    if (framebuffer_size != color_buffer->size()) {
                        notf_throw(runtime_error, "All RenderBuffers attached to the same FrameBuffer must be of the "
                                                  "same size.");
                    }
                }
                else {
                    framebuffer_size = color_buffer->size();
                }

                has_any_attachment = true;
            }
        }
        else if (std::holds_alternative<TexturePtr>(color_target)) {
            if (std::get<TexturePtr>(color_target)) {
                has_any_attachment = true;
            }
        }
        else {
            assert(0); // invalid variant
        }
    }

    // depth attachment
    RenderBufferPtr depth_buffer;
    if (std::holds_alternative<RenderBufferPtr>(m_args.depth_target)) {
        if ((depth_buffer = std::get<RenderBufferPtr>(m_args.depth_target))) {
            if (depth_buffer->type() != RenderBuffer::Type::DEPTH
                && depth_buffer->type() != RenderBuffer::Type::DEPTH_STENCIL) {
                notf_throw_format(runtime_error, "Cannot attach a RenderBuffer of type "
                                                     << type_to_str(depth_buffer->type()) << " as depth buffer");
            }

            if (framebuffer_size.is_valid()) {
                if (framebuffer_size != depth_buffer->size()) {
                    notf_throw(runtime_error, "All RenderBuffers attached to the same FrameBuffer must be of the "
                                              "same size.");
                }
            }
            else {
                framebuffer_size = depth_buffer->size();
            }

            has_any_attachment = true;
        }
    }
    else if (std::holds_alternative<TexturePtr>(m_args.depth_target)) {
        if (std::get<TexturePtr>(m_args.depth_target)) {
            has_any_attachment = true;
        }
    }
    else {
        assert(0); // invalid variant
    }

    // stencil attachment
    if (m_args.stencil_target) {
        if (m_args.stencil_target->type() != RenderBuffer::Type::STENCIL
            && m_args.stencil_target->type() != RenderBuffer::Type::DEPTH_STENCIL) {
            notf_throw_format(runtime_error, "Cannot attach a RenderBuffer of type "
                                                 << type_to_str(m_args.stencil_target->type()) << " as stencil buffer");
        }

        if (framebuffer_size.is_valid()) {
            if (framebuffer_size != m_args.stencil_target->size()) {
                notf_throw(runtime_error, "All RenderBuffers attached to the same FrameBuffer must be of the "
                                          "same size.");
            }
        }

        has_any_attachment = true;
    }

    if (!has_any_attachment) {
        notf_throw(runtime_error, "Cannot construct FrameBuffer without a single valid attachment");
    }

    if (depth_buffer && m_args.stencil_target) {
        if (depth_buffer != m_args.stencil_target) {
            notf_throw(runtime_error, "FrameBuffers with both depth and stencil attachments have to refer to the same "
                                      "RenderBuffer");
        }
        if (depth_buffer->internal_format() != GL_DEPTH24_STENCIL8
            && depth_buffer->internal_format() != GL_DEPTH32F_STENCIL8) {
            notf_throw(runtime_error,
                       "FrameBuffers with both depth and stencil attachments must have an internal format "
                       "packing both depth and stencil into one value");
        }
    }
}

} // namespace notf
