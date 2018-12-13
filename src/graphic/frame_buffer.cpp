#include "notf/graphic/frame_buffer.hpp"

#include <algorithm>
#include <set>
#include <sstream>

#include "notf/meta/assert.hpp"
#include "notf/meta/exception.hpp"
#include "notf/meta/log.hpp"

#include "notf/graphic/gl_errors.hpp"
#include "notf/graphic/graphics_system.hpp"
#include "notf/graphic/texture.hpp"

namespace { // anonymous
NOTF_USING_NAMESPACE;

/// Human-readable name of a RenderBuffer type.
const char* type_to_str(const RenderBuffer::Type type) {
    switch (type) {
    case RenderBuffer::Type::COLOR: return "COLOR";
    case RenderBuffer::Type::DEPTH: return "DEPTH";
    case RenderBuffer::Type::STENCIL: return "STENCIL";
    case RenderBuffer::Type::DEPTH_STENCIL: return "DEPTH_STENCIL";
    }
    NOTF_ASSERT(false);
    return ""; // to squelch warnings
}

/// Humand-readable error string for status returned by glCheckFramebufferStatus.
const char* status_to_str(const GLenum status) {
    switch (status) {
    case (GL_FRAMEBUFFER_UNDEFINED): return "GL_FRAMEBUFFER_UNDEFINED";
    case (GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT): return "GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT";
    case (GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT): return "GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT";
    case (GL_FRAMEBUFFER_UNSUPPORTED): return "GL_FRAMEBUFFER_UNSUPPORTED";
    case (GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE): return "GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE";
    default: NOTF_ASSERT(false);
    }
    return nullptr;
}

} // namespace

// render buffer ==================================================================================================== //

NOTF_OPEN_NAMESPACE

RenderBuffer::RenderBuffer(Args&& args) : m_args(std::move(args)) {
    // check arguments
    const auto& env = TheGraphicsSystem::get_environment();
    const short max_size = static_cast<short>(env.max_render_buffer_size);
    if (!m_args.size.is_valid() || m_args.size.get_area() == 0 || m_args.size.height() > max_size
        || m_args.size.width() > max_size) {
        NOTF_THROW(ValueError, "Invalid render buffer size {}, maximum is {}x{}", args.size, max_size, max_size);
    }
    if (m_args.internal_format == 0) { NOTF_THROW(ValueError, "Invalid internal format for RenderBuffer: 0"); }
    if (m_args.type == Type::COLOR) {
        _assert_color_format(m_args.internal_format);
    } else {
        _assert_depth_stencil_format(m_args.internal_format);
    }

    // generate new renderbuffer id
    NOTF_CHECK_GL(glGenRenderbuffers(1, &m_id.data()));
    if (m_id == 0) { NOTF_THROW(OpenGLError, "Could not allocate new RenderBuffer"); }
}

RenderBuffer::~RenderBuffer() { _deallocate(); }

void RenderBuffer::_assert_color_format(const GLenum internal_format) {
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
    case GL_RGBA32UI: break;
    default: NOTF_THROW(ValueError, "Invalid internal format for a color buffer: {}", internal_format);
    }
}

void RenderBuffer::_assert_depth_stencil_format(const GLenum internal_format) {
    switch (internal_format) {
    case GL_DEPTH_COMPONENT16:
    case GL_DEPTH_COMPONENT24:
    case GL_DEPTH_COMPONENT32F:
    case GL_DEPTH24_STENCIL8:
    case GL_DEPTH32F_STENCIL8:
    case GL_STENCIL_INDEX8: break;
    default: NOTF_THROW(ValueError, "Invalid internal format for a depth or stencil buffer: {}", internal_format);
    }
}

void RenderBuffer::_deallocate() {
    if (m_id != 0) { NOTF_CHECK_GL(glDeleteRenderbuffers(1, &m_id.value())); }
    m_id = RenderBufferId::invalid();
}

// frame buffer ===================================================================================================== //

void FrameBuffer::Args::set_color_target(const ushort id, ColorTarget target) {
    auto it = std::find_if(color_targets.begin(), color_targets.end(),
                           [id](const std::pair<ushort, ColorTarget>& target) -> bool { return target.first == id; });
    if (it == color_targets.end()) {
        color_targets.emplace_back(std::make_pair(id, std::move(target)));
    } else {
        it->second = std::move(target);
    }
}

FrameBuffer::FrameBuffer(Args&& args) : m_id(0), m_args(args) {
    _validate_arguments();

    { // generate new framebuffer id
        FrameBufferId::underlying_t id = 0;
        NOTF_CHECK_GL(glGenFramebuffers(1, &id));
        m_id = id;
        if (m_id == 0) { NOTF_THROW(OpenGLError, "Could not allocate new FrameBuffer"); }
    }

    // define framebuffer
    NOTF_CHECK_GL(glBindFramebuffer(GL_FRAMEBUFFER, m_id.value()));

    for (const auto& numbered_color_target : m_args.color_targets) {
        const ushort target_id = numbered_color_target.first;
        const auto& color_target = numbered_color_target.second;

        if (std::holds_alternative<RenderBufferPtr>(color_target)) {
            if (const auto& color_buffer = std::get<RenderBufferPtr>(color_target)) {
                NOTF_CHECK_GL(glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + target_id,
                                                        GL_RENDERBUFFER, color_buffer->get_id().value()));
            }
        } else if (const auto& color_texture = std::get<TexturePtr>(color_target)) {
            NOTF_CHECK_GL(glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + target_id,
                                                 color_texture->get_target(), color_texture->get_id().value(),
                                                 /* level = */ 0));
        }
    }

    bool has_depth = false;
    if (std::holds_alternative<RenderBufferPtr>(m_args.depth_target)) {
        if (const auto& depth_buffer = std::get<RenderBufferPtr>(m_args.depth_target)) {
            NOTF_CHECK_GL(glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER,
                                                    depth_buffer->get_id().value()));
            has_depth = true;
        }
    } else if (const auto& depth_texture = std::get<TexturePtr>(m_args.depth_target)) {
        NOTF_CHECK_GL(glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, depth_texture->get_target(),
                                             depth_texture->get_id().value(), /* level = */ 0));
        has_depth = true;
    }

    if (m_args.stencil_target) {
        NOTF_CHECK_GL(glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_STENCIL_ATTACHMENT, GL_RENDERBUFFER,
                                                m_args.stencil_target->get_id().value()));
    }

    { // make sure the frame buffer is valid
        GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
        const bool has_stencil = (m_args.stencil_target == nullptr);
        if (status == GL_FRAMEBUFFER_COMPLETE) {
            NOTF_LOG_INFO("Created new FrameBuffer {} with {} color attachments{}{}{}", m_id,
                          m_args.color_targets.size(), (has_depth ? (has_depth && has_stencil ? "," : " and") : ""),
                          (has_depth ? " a depth attachment" : ""), (has_stencil ? " and a stencil attachment" : ""));
        } else {
            NOTF_THROW(OpenGLError, "{}", status_to_str(status));
        }
    }

    NOTF_CHECK_GL(glBindFramebuffer(GL_FRAMEBUFFER, 0));
}

FrameBufferPtr FrameBuffer::create(Args&& args) {
    FrameBufferPtr framebuffer = _create_shared(std::move(args));
    TheGraphicsSystem::AccessFor<FrameBuffer>::register_new(framebuffer);
    return framebuffer;
}

FrameBuffer::~FrameBuffer() { _deallocate(); }

const TexturePtr& FrameBuffer::get_color_texture(const ushort id) {
    auto itr = std::find_if(std::begin(m_args.color_targets), std::end(m_args.color_targets),
                            [id](const std::pair<ushort, ColorTarget>& target) -> bool { return target.first == id; });
    try {
        if (itr != std::end(m_args.color_targets)) { return std::get<TexturePtr>(itr->second); }
    }
    catch (std::bad_variant_access&) {
        /* ignore */
    }
    NOTF_THROW(RunTimeError, "FrameBuffer has no color attachment: {}", id);
}

void FrameBuffer::_deallocate() {
    if (m_id == 0) { return; }

    auto render_buffer_access = RenderBuffer::AccessFor<FrameBuffer>();

    // deallocate all attached RenderBuffers
    for (const auto& numbered_color_target : m_args.color_targets) {
        const auto& color_target = numbered_color_target.second;
        if (std::holds_alternative<RenderBufferPtr>(color_target)) {
            NOTF_ASSERT(std::get<RenderBufferPtr>(color_target)); // RenderBuffer pointer should never be empty
            render_buffer_access.deallocate(*std::get<RenderBufferPtr>(color_target));
        }
    }

    if (std::holds_alternative<RenderBufferPtr>(m_args.depth_target)) {
        NOTF_ASSERT(std::get<RenderBufferPtr>(m_args.depth_target)); // RenderBuffer pointer should never be empty
        render_buffer_access.deallocate(*std::get<RenderBufferPtr>(m_args.depth_target));
    }

    if (m_args.stencil_target) { render_buffer_access.deallocate(*m_args.stencil_target); }

    // shed all owning pointers
    m_args = Args{};

    // deallocate yourself
    NOTF_CHECK_GL(glDeleteFramebuffers(1, &m_id.value()));
    m_id = FrameBufferId::invalid();
}

/// For the rules, check:
/// https://www.khronos.org/opengl/wiki/Framebuffer_Object#Framebuffer_Completeness
void FrameBuffer::_validate_arguments() const {
    bool has_any_attachment = false;             // is there at least one valid attachment for the frame buffer?
    std::set<ushort> used_targets;               // is any color target used twice?
    Size2s framebuffer_size = Size2s::invalid(); // do all framebuffers have the same size?

    // color attachments
    for (const auto& numbered_color_target : m_args.color_targets) {
        const ushort target_id = numbered_color_target.first;
        if (used_targets.count(target_id)) { NOTF_THROW(ValueError, "Duplicate color attachment id: {}", target_id); }
        used_targets.insert(target_id);

        const auto& color_target = numbered_color_target.second;
        if (std::holds_alternative<RenderBufferPtr>(color_target)) {
            if (const auto& color_buffer = std::get<RenderBufferPtr>(color_target)) {
                if (color_buffer->get_type() != RenderBuffer::Type::COLOR) {
                    NOTF_THROW(ValueError, "Cannot attach a RenderBuffer of type {} as color buffer",
                               type_to_str(color_buffer->get_type()));
                }

                if (framebuffer_size.is_valid()) {
                    if (framebuffer_size != color_buffer->get_size()) {
                        NOTF_THROW(ValueError,
                                   "All RenderBuffers attached to the same FrameBuffer must be of the same size.");
                    }
                } else {
                    framebuffer_size = color_buffer->get_size();
                }

                has_any_attachment = true;
            }
        } else if (std::holds_alternative<TexturePtr>(color_target)) {
            if (std::get<TexturePtr>(color_target)) { has_any_attachment = true; }
        } else {
            NOTF_ASSERT(false); // invalid variant
        }
    }

    // depth attachment
    RenderBufferPtr depth_buffer;
    if (std::holds_alternative<RenderBufferPtr>(m_args.depth_target)) {
        if ((depth_buffer = std::get<RenderBufferPtr>(m_args.depth_target))) {
            if (depth_buffer->get_type() != RenderBuffer::Type::DEPTH
                && depth_buffer->get_type() != RenderBuffer::Type::DEPTH_STENCIL) {
                NOTF_THROW(ValueError, "Cannot attach a RenderBuffer of type {} as depth buffer",
                           type_to_str(depth_buffer->get_type()));
            }

            if (framebuffer_size.is_valid()) {
                if (framebuffer_size != depth_buffer->get_size()) {
                    NOTF_THROW(ValueError, "All RenderBuffers attached to the same FrameBuffer must be of the "
                                           "same size.");
                }
            } else {
                framebuffer_size = depth_buffer->get_size();
            }

            has_any_attachment = true;
        }
    } else if (std::holds_alternative<TexturePtr>(m_args.depth_target)) {
        if (std::get<TexturePtr>(m_args.depth_target)) { has_any_attachment = true; }
    } else {
        NOTF_ASSERT(false); // invalid variant
    }

    // stencil attachment
    if (m_args.stencil_target) {
        if (m_args.stencil_target->get_type() != RenderBuffer::Type::STENCIL
            && m_args.stencil_target->get_type() != RenderBuffer::Type::DEPTH_STENCIL) {
            NOTF_THROW(ValueError, "Cannot attach a RenderBuffer of type {} as stencil buffer",
                       type_to_str(m_args.stencil_target->get_type()));
        }

        if (framebuffer_size.is_valid()) {
            if (framebuffer_size != m_args.stencil_target->get_size()) {
                NOTF_THROW(ValueError, "All RenderBuffers attached to the same FrameBuffer must be of the same size.");
            }
        }

        has_any_attachment = true;
    }

    if (!has_any_attachment) {
        NOTF_THROW(ValueError, "Cannot construct FrameBuffer without a single valid attachment");
    }

    if (depth_buffer && m_args.stencil_target) {
        if (depth_buffer != m_args.stencil_target) {
            NOTF_THROW(ValueError,
                       "FrameBuffers with both depth and stencil attachments have to refer to the same RenderBuffer");
        }
        if (depth_buffer->get_internal_format() != GL_DEPTH24_STENCIL8
            && depth_buffer->get_internal_format() != GL_DEPTH32F_STENCIL8) {
            NOTF_THROW(ValueError, "FrameBuffers with both depth and stencil attachments must have an internal format "
                                   "packing both depth and stencil into one value");
        }
    }
}

NOTF_CLOSE_NAMESPACE
