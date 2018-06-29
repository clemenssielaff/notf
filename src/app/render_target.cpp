#include "app/render_target.hpp"

#include <sstream>

#include "graphics/core/frame_buffer.hpp"
#include "graphics/core/graphics_context.hpp"
#include "graphics/core/texture.hpp"
#include "graphics/renderer/renderer.hpp"

NOTF_OPEN_NAMESPACE

RenderTarget::RenderTarget(GraphicsContext& context, Args&& args)
    : m_scene(std::move(args.scene)), m_renderer(std::move(args.renderer))
{
    // create the texture arguments
    Texture::Args texture_args;
    texture_args.is_linear = true; // important
    texture_args.anisotropy = args.anisotropy;

    if (args.create_mipmaps) {
        texture_args.min_filter = Texture::MinFilter::LINEAR_MIPMAP_LINEAR;
        texture_args.mag_filter = Texture::MagFilter::LINEAR;
        texture_args.create_mipmaps = true;
    }
    else {
        texture_args.min_filter = Texture::MinFilter::NEAREST;
        texture_args.mag_filter = Texture::MagFilter::NEAREST;
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
    {
        std::stringstream ss;
        ss << "RenderTargetTexture#" << this;
        framebuffer_args.set_color_target(0, Texture::create_empty(context, ss.str(), args.size, texture_args));
    }

    m_framebuffer = FrameBuffer::create(context, std::move(framebuffer_args));
}

RenderTarget::~RenderTarget() = default;

const TexturePtr& RenderTarget::texture() const { return m_framebuffer->get_color_texture(0); }

void RenderTarget::clean()
{
    if (!is_dirty()) {
        return;
    }

    // prepare the graphic state
    GraphicsContext& context = m_framebuffer->get_context();
    const auto framebuffer_guard = context.bind_framebuffer(m_framebuffer);
    context.set_render_area(texture()->get_size());
    context.clear(Color::black());

    // render everything
    Renderer::Access<RenderTarget>::render(*m_renderer, m_scene.get());
}

NOTF_CLOSE_NAMESPACE
