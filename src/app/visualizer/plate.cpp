#include "app/visualizer/plate.hpp"

#include <sstream>

#include "app/visualizer/visualizer.hpp"
#include "graphics/core/frame_buffer.hpp"
#include "graphics/core/graphics_context.hpp"
#include "graphics/core/texture.hpp"

NOTF_OPEN_NAMESPACE

Plate::Plate(GraphicsContext& context, Args&& args)
    : m_scene(std::move(args.scene)), m_visualizer(std::move(args.visualizer))
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
        ss << "Plate#" << this;
        framebuffer_args.set_color_target(0, Texture::create_empty(context, ss.str(), args.size, texture_args));
    }

    m_framebuffer = FrameBuffer::create(context, std::move(framebuffer_args));
}

Plate::~Plate() = default;

const TexturePtr& Plate::texture() const { return m_framebuffer->get_color_texture(0); }

void Plate::clean()
{
    if (!is_dirty()) {
        return;
    }

    // prepare the graphic state
    GraphicsContext& context = m_framebuffer->get_context();
    const auto framebuffer_guard = context.bind_framebuffer(m_framebuffer);
    context.set_render_area(texture()->get_size());
    context.clear(Color::black());

    // draw everything
    Visualizer::Access<Plate>::visualize(*m_visualizer, m_scene.get());
}

NOTF_CLOSE_NAMESPACE
