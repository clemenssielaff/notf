#include "notf/app/graph/visualizer.hpp"

#include "notf/app/graph/window.hpp"

#include "notf/graphic/frame_buffer.hpp"
#include "notf/graphic/graphics_context.hpp"
#include "notf/graphic/texture.hpp"

NOTF_OPEN_NAMESPACE

// plate ============================================================================================================ //

Visualizer::Plate::Plate(Args&& args) : m_scene(std::move(args.scene)), m_visualizer(std::move(args.visualizer)) {
    // create the texture arguments
    Texture::Args texture_args;
    texture_args.is_linear = true; // important
    texture_args.anisotropy = args.anisotropy;

    if (args.create_mipmaps) {
        texture_args.min_filter = Texture::MinFilter::LINEAR_MIPMAP_LINEAR;
        texture_args.mag_filter = Texture::MagFilter::LINEAR;
        texture_args.create_mipmaps = true;
    } else {
        texture_args.min_filter = Texture::MinFilter::NEAREST;
        texture_args.mag_filter = Texture::MagFilter::NEAREST;
        texture_args.create_mipmaps = false;
    }

    if (args.has_transparency) {
        texture_args.format = Texture::Format::RGBA;
    } else {
        texture_args.format = Texture::Format::RGB;
    }

    // create the framebuffer
    static size_t plate_counter = 0;
    ++plate_counter;
    std::string name = fmt::format("Plate#{}", plate_counter);
    FrameBuffer::Args framebuffer_args;
    framebuffer_args.set_color_target(0, Texture::create_empty(name, args.size, texture_args));
    m_framebuffer = FrameBuffer::create(m_visualizer->get_context(), std::move(name), std::move(framebuffer_args));
}

Visualizer::Plate::~Plate() = default;

const TexturePtr& Visualizer::Plate::get_texture() const { return m_framebuffer->get_color_texture(0); }

void Visualizer::Plate::clean() {
    if (!is_dirty()) { return; }

    // prepare the graphic state
    GraphicsContext& context = m_visualizer->get_context();
    NOTF_GUARD(context.make_current());
    context->framebuffer = m_framebuffer;
    context->framebuffer.set_render_area(get_texture()->get_size());
    context->framebuffer.clear(Color::black());

    // draw everything
    m_visualizer->visualize(m_scene.get());
}

// plate ============================================================================================================ //

Visualizer::Visualizer(const Window& window) : m_context(window.get_graphics_context()) {}

Visualizer::~Visualizer() = default;

NOTF_CLOSE_NAMESPACE
