#include "app/renderer/render_target.hpp"

#include "app/renderer/graphics_producer.hpp"
#include "app/scene/scene_manager.hpp"
#include "graphics/core/frame_buffer.hpp"
#include "graphics/core/graphics_context.hpp"
#include "graphics/core/texture.hpp"
#include "utils/make_smart_enabler.hpp"

namespace notf {

RenderTarget::RenderTarget(SceneManager& scene_manager, Args&& args)
    : m_id(_next_id())
    , m_context(*scene_manager.graphics_context())
    , m_name(std::move(args.name))
    , m_framebuffer()
    , m_producer(std::move(args.producer))
    , m_is_dirty(true)
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
    framebuffer_args.set_color_target(0, Texture::create_empty(m_context, m_name, std::move(args.size),
                                                               std::move(texture_args)));
    m_framebuffer = FrameBuffer::create(m_context, std::move(framebuffer_args));
}

RenderTargetPtr RenderTarget::create(SceneManager& manager, Args&& args)
{
#ifdef NOTF_DEBUG
    auto result = RenderTargetPtr(new RenderTarget(manager, std::move(args)));
#else
    auto result = std::make_shared<make_shared_enabler<RenderTarget>>(manager, std::move(args));
#endif
    SceneManager::Access<RenderTarget>(manager).register_new(result);
    return result;
}

const TexturePtr& RenderTarget::texture() const { return m_framebuffer->color_texture(0); }

void RenderTarget::clean()
{
    if (!is_dirty()) {
        return;
    }

    // prepare the graphic state
    const auto framebuffer_guard = m_context.bind_framebuffer(m_framebuffer);
    m_context.set_render_area(texture()->size());
    m_context.clear(Color::black());

    // render everything
    GraphicsProducer::Access<RenderTarget>(*m_producer).render();
}

RenderTargetId RenderTarget::_next_id()
{
    static RenderTargetId::underlying_t next = 1;
    return RenderTargetId(next++);
}

} // namespace notf
