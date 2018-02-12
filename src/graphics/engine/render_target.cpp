#include "graphics/engine/render_target.hpp"

#include "graphics/core/frame_buffer.hpp"
#include "graphics/core/graphics_context.hpp"
#include "graphics/core/texture.hpp"
#include "utils/make_smart_enabler.hpp"

namespace notf {

RenderTarget::RenderTarget(Args&& args) : m_context(args.context), m_name(std::move(args.name)), m_framebuffer()
{
    // create the texture arguments
    Texture::Args texture_args;
    texture_args.is_linear  = true; // important
    texture_args.anisotropy = args.anisotropy;

    if (args.create_mipmaps) {
        texture_args.min_filter     = Texture::MinFilter::LINEAR_MIPMAP_LINEAR;
        texture_args.mag_filter     = Texture::MagFilter::LINEAR;
        texture_args.create_mipmaps = true;
    }
    else {
        texture_args.min_filter     = Texture::MinFilter::NEAREST;
        texture_args.mag_filter     = Texture::MagFilter::NEAREST;
        texture_args.create_mipmaps = false;
    }

    if (args.has_transparency) {
        texture_args.format = Texture::Format::RGBA;
    }
    else {
        texture_args.format = Texture::Format::RGB;
    }

    // create the framebuffer
    FrameBuffer::Args framebuffer_args;
    framebuffer_args.set_color_target(0, Texture::create_empty(args.context, m_name, std::move(args.size),
                                                               std::move(texture_args)));
    m_framebuffer = FrameBuffer::create(args.context, std::move(framebuffer_args));
}

RenderTargetPtr RenderTarget::create(Args&& args)
{
#ifdef NOTF_DEBUG
    return RenderTargetPtr(new RenderTarget(std::move(args)));
#else
    return std::make_shared<make_shared_enabler<RenderTarget>>(std::move(args));
#endif
}

void RenderTarget::bind_as_framebuffer() { m_context.bind_framebuffer(m_framebuffer); }

void RenderTarget::bind_as_texture(uint slot) { m_context.bind_texture(m_framebuffer->color_texture(0).get(), slot); }

} // namespace notf
