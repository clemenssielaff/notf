#include "notf/graphic/frame_buffer.hpp"

#include <set>

#include "notf/meta/log.hpp"

#include "notf/app/resource_manager.hpp"

#include "notf/graphic/graphics_system.hpp"
#include "notf/graphic/texture.hpp"

NOTF_USING_NAMESPACE;

namespace { // anonymous

/// Human-readable name of a RenderBuffer type.
const char* type_to_str(const RenderBuffer::Type type) {
    switch (type) {
    case RenderBuffer::Type::COLOR: return "COLOR";
    case RenderBuffer::Type::DEPTH: return "DEPTH";
    case RenderBuffer::Type::STENCIL: return "STENCIL";
    case RenderBuffer::Type::DEPTH_STENCIL: return "DEPTH_STENCIL";
    default: NOTF_ASSERT(false);
    }
}

/// Humand-readable error string for status returned by glCheckFramebufferStatus.
const char* status_to_str(const GLenum status) {
    switch (status) {
    case (GL_FRAMEBUFFER_UNDEFINED): return "GL_FRAMEBUFFER_UNDEFINED";
    case (GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT): return "GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT";
    case (GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT): return "GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT";
    case (GL_FRAMEBUFFER_UNSUPPORTED): return "GL_FRAMEBUFFER_UNSUPPORTED";
    case (GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE): return "GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE";
    default: return "Unknown OpenGL framebuffer error";
    }
}

/// Checks, whether the given format is a valid internal format for a color render buffer.
/// @throws ValueError  ...if it isn't.
void assert_color_format(const GLenum internal_format) {
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
    case GL_RGBA32UI: return;
    default: NOTF_THROW(ValueError, "Invalid internal format for a color buffer: {}", internal_format);
    }
}

/// Checks, whether the given format is a valid internal format for a depth or stencil render buffer.
/// @throws ValueError  ...if it isn't.
void assert_depth_stencil_format(const GLenum internal_format) {
    switch (internal_format) {
    case GL_DEPTH_COMPONENT16:
    case GL_DEPTH_COMPONENT24:
    case GL_DEPTH_COMPONENT32F:
    case GL_DEPTH24_STENCIL8:
    case GL_DEPTH32F_STENCIL8:
    case GL_STENCIL_INDEX8: return;
    default: NOTF_THROW(ValueError, "Invalid internal format for a depth or stencil buffer: {}", internal_format);
    }
}

} // namespace

// render buffer ==================================================================================================== //

RenderBuffer::RenderBuffer(std::string name, Args args) : m_name(std::move(name)), m_args(std::move(args)) {
    // check arguments
    const auto& env = TheGraphicsSystem::get_environment();
    const short max_size = static_cast<short>(env.max_render_buffer_size);
    if (!m_args.size.is_valid() || m_args.size.get_area() == 0 || m_args.size.height() > max_size
        || m_args.size.width() > max_size) {
        NOTF_THROW(ValueError, "Invalid RenderBuffer size {}, maximum is {}x{}", args.size, max_size, max_size);
    }
    if (m_args.internal_format == 0) { NOTF_THROW(ValueError, "Invalid internal format for RenderBuffer: 0"); }
    if (m_args.type == Type::COLOR) {
        assert_color_format(m_args.internal_format);
    } else {
        assert_depth_stencil_format(m_args.internal_format);
    }
    if (m_args.samples < 0) {
        NOTF_THROW(ValueError, "Invalid sample value {}, number of samples cannot be less than zero", m_args.samples);
    } else if (m_args.samples > env.max_sample_count) {
        NOTF_THROW(ValueError, "Invalid sample value {}, maximum number of samples is {}", m_args.samples,
                   env.max_sample_count);
    }

    // generate new renderbuffer id
    NOTF_CHECK_GL(glGenRenderbuffers(1, &m_id.data()));
    if (m_id == 0) { NOTF_THROW(OpenGLError, "Could not allocate new RenderBuffer"); }

    // bind and eventually release the renderbuffer after we are done allocating it
    struct RenderBufferGuard {
        RenderBufferGuard(const GLuint id) { glBindRenderbuffer(GL_RENDERBUFFER, id); }
        ~RenderBufferGuard() { glBindRenderbuffer(GL_RENDERBUFFER, 0); }
    } guard(m_id.get_value());

    // allocate storage for the renderbuffer
    if (m_args.samples == 0) {
        glRenderbufferStorage(GL_RENDERBUFFER, m_args.internal_format, m_args.size.width(), m_args.size.height());
    } else {
        glRenderbufferStorageMultisample(GL_RENDERBUFFER, m_args.samples, m_args.internal_format, m_args.size.width(),
                                         m_args.size.height());
    }

    NOTF_LOG_TRACE("Created RenderBuffer \"{}\"", m_name);
}

RenderBufferPtr RenderBuffer::create(std::string name, Args args) {
    RenderBufferPtr renderbuffer = _create_shared(name, std::move(args));
    TheGraphicsSystem::AccessFor<RenderBuffer>::register_new(renderbuffer);
    ResourceManager::get_instance().get_type<RenderBuffer>().set(std::move(name), renderbuffer);
    return renderbuffer;
}

RenderBuffer::~RenderBuffer() { _deallocate(); }

void RenderBuffer::_deallocate() {
    if (!m_id.is_valid()) { return; }

    NOTF_GUARD(TheGraphicsSystem()->make_current());

    // deallocate the renderbuffer
    NOTF_CHECK_GL(glDeleteRenderbuffers(1, &m_id.get_value()));
    m_id = RenderBufferId::invalid();

    NOTF_LOG_TRACE("Deleted RenderBuffer \"{}\"", m_name);
}

// frame buffer args ================================================================================================ //

void FrameBuffer::Args::set_color_target(const ushort id, ColorTarget target) {
    auto it = std::find_if(color_targets.begin(), color_targets.end(),
                           [id](const std::pair<ushort, ColorTarget>& target) -> bool { return target.first == id; });
    if (it == color_targets.end()) {
        color_targets.emplace_back(std::make_pair(id, std::move(target)));
    } else {
        it->second = std::move(target);
    }
}

// frame buffer ===================================================================================================== //

FrameBuffer::FrameBuffer(GraphicsContext& context, std::string name, Args args, bool is_default)
    : m_context(context), m_name(std::move(name)), m_is_default(is_default), m_args(std::move(args)) {
    if (m_is_default) { return; }
    _validate_arguments();

    NOTF_GUARD(m_context.make_current());

    // generate new framebuffer id
    NOTF_CHECK_GL(glGenFramebuffers(1, &m_id.data()));
    if (!m_id.is_valid()) { NOTF_THROW(OpenGLError, "Could not allocate new FrameBuffer"); }

    // bind and eventually release the framebuffer after we are done defining it
    struct RenderBufferGuard {
        RenderBufferGuard(const GLuint id) { glBindFramebuffer(GL_FRAMEBUFFER, id); }
        ~RenderBufferGuard() { glBindFramebuffer(GL_FRAMEBUFFER, 0); }
    } guard(m_id.get_value());

    // color attachments
    size_t color_attachment_count = 0;
    for (const auto& numbered_color_target : m_args.color_targets) {
        const ushort target_id = numbered_color_target.first;
        const ColorTarget& color_target = numbered_color_target.second;

        if (const TexturePtr* color_texture = std::get_if<TexturePtr>(&color_target)) {
            if (*color_texture) {
                NOTF_CHECK_GL(glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + target_id,
                                                     (*color_texture)->get_target(),
                                                     (*color_texture)->get_id().get_value(),
                                                     /* level = */ 0));
                ++color_attachment_count;
            }
        } else if (const RenderBufferPtr* color_buffer = std::get_if<RenderBufferPtr>(&color_target)) {
            if (*color_buffer) {
                NOTF_CHECK_GL(glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + target_id,
                                                        GL_RENDERBUFFER, (*color_buffer)->get_id().get_value()));
                ++color_attachment_count;
            }
        }
    }

    // depth attachment
    bool has_depth = true;
    if (const TexturePtr* depth_texture = std::get_if<TexturePtr>(&m_args.depth_target)) {
        if (*depth_texture) {
            NOTF_CHECK_GL(glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, (*depth_texture)->get_target(),
                                                 (*depth_texture)->get_id().get_value(), /* level = */ 0));
        }
    } else if (const RenderBufferPtr* depth_buffer = std::get_if<RenderBufferPtr>(&m_args.depth_target)) {
        if (*depth_buffer) {
            NOTF_CHECK_GL(glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER,
                                                    (*depth_buffer)->get_id().get_value()));
        }
    } else {
        has_depth = false;
    }

    // stencil attachment
    bool has_stencil = false;
    if (m_args.stencil_target) {
        NOTF_CHECK_GL(glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_STENCIL_ATTACHMENT, GL_RENDERBUFFER,
                                                m_args.stencil_target->get_id().get_value()));
        has_stencil = true;
    }

    // validate and log
    if (GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER); status == GL_FRAMEBUFFER_COMPLETE) {
        NOTF_LOG_INFO("Created new FrameBuffer {} with {} color attachments{}", m_id, color_attachment_count,
                      (has_depth ? (has_stencil ? ", a depth and a stencil attachment" : " and a depth attachment") :
                                   (has_stencil ? " and a stencil attachment" : "")));
    } else {
        NOTF_THROW(OpenGLError, "{}", status_to_str(status));
    }
}

FrameBufferPtr FrameBuffer::create(GraphicsContext& context, std::string name, Args args) {
    FrameBufferPtr framebuffer = _create_shared(context, name, std::move(args));
    GraphicsContext::AccessFor<FrameBuffer>::register_new(context, framebuffer);
    ResourceManager::get_instance().get_type<FrameBuffer>().set(std::move(name), framebuffer);
    return framebuffer;
}

FrameBuffer::~FrameBuffer() { _deallocate(); }

bool FrameBuffer::is_valid() const {
    if (m_is_default) {
        return true;
    } else {
        return m_id.is_valid();
    }
}

const TexturePtr& FrameBuffer::get_color_texture(const ushort id) {
    auto itr = std::find_if(std::begin(m_args.color_targets), std::end(m_args.color_targets),
                            [id](const std::pair<ushort, ColorTarget>& target) -> bool { return target.first == id; });
    if (itr != std::end(m_args.color_targets) && std::holds_alternative<TexturePtr>(itr->second)) {
        return std::get<TexturePtr>(itr->second);
    } else {
        NOTF_THROW(RunTimeError, "FrameBuffer has no color attachment: {}", id);
    }
}

void FrameBuffer::_deallocate() {
    if (!m_id.is_valid()) { return; }

    NOTF_GUARD(m_context.make_current());

    // shed all owning pointers
    m_args = Args{};

    // deallocate the framebuffer
    NOTF_CHECK_GL(glDeleteFramebuffers(1, &m_id.get_value()));
    m_id = FrameBufferId::invalid();

    NOTF_LOG_TRACE("Deleted FrameBuffer \"{}\"", m_name);
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
        if (const TexturePtr* color_texture = std::get_if<TexturePtr>(&color_target)) {
            if (*color_texture) { has_any_attachment = true; }
        } else if (const RenderBufferPtr* color_buffer = std::get_if<RenderBufferPtr>(&color_target)) {
            if (*color_buffer) {
                if ((*color_buffer)->get_type() != RenderBuffer::Type::COLOR) {
                    NOTF_THROW(ValueError, "Cannot attach a RenderBuffer of type {} as color buffer",
                               type_to_str((*color_buffer)->get_type()));
                }

                if (framebuffer_size.is_valid()) {
                    if (framebuffer_size != (*color_buffer)->get_size()) {
                        NOTF_THROW(ValueError,
                                   "All RenderBuffers attached to the same FrameBuffer must be of the same size.");
                    }
                } else {
                    framebuffer_size = (*color_buffer)->get_size();
                }

                has_any_attachment = true;
            }
        } else {
            NOTF_ASSERT(false); // invalid variant
        }
    }

    // depth attachment
    RenderBufferPtr depth_buffer;
    if (const TexturePtr* depth_texture = std::get_if<TexturePtr>(&m_args.depth_target)) {
        if (*depth_texture) { has_any_attachment = true; }
    } else if (const RenderBufferPtr* depth_buffer = std::get_if<RenderBufferPtr>(&m_args.depth_target)) {
        if (*depth_buffer) {
            if ((*depth_buffer)->get_type() != RenderBuffer::Type::DEPTH
                && (*depth_buffer)->get_type() != RenderBuffer::Type::DEPTH_STENCIL) {
                NOTF_THROW(ValueError, "Cannot attach a RenderBuffer of type {} as depth buffer",
                           type_to_str((*depth_buffer)->get_type()));
            }

            if (framebuffer_size.is_valid()) {
                if (framebuffer_size != (*depth_buffer)->get_size()) {
                    NOTF_THROW(ValueError,
                               "All RenderBuffers attached to the same FrameBuffer must be of the same size.");
                }
            } else {
                framebuffer_size = (*depth_buffer)->get_size();
            }

            has_any_attachment = true;
        }
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

// accessors -------------------------------------------------------------------------------------------------------- //

FrameBufferPtr Accessor<FrameBuffer, GraphicsContext>::create_default(GraphicsContext& context) {
    return FrameBuffer::_create_shared(context, context.get_name(), FrameBuffer::Args{}, /*is_default = */ true);
}
