#pragma once

#include "common/forwards.hpp"
#include "common/size2.hpp"

namespace notf {

/// Has a framebuffer with a single texture attached as color target.
class RenderTarget {

    // fields --------------------------------------------------------------------------------------------------------//
public:
    struct Args {
        /// Graphics context.
        GraphicsContext& context;

        /// Name of the RenderTarget, unique within the RenderManager.
        std::string name;

        /// Size of the RenderTarget.
        Size2i size;

        /// Set to `true`, if this FrameBuffer has transparency.
        bool has_transparency = false;

        /// If you don't plan on transforming the RenderTarget before displaying it on screen, leave this set to `false`
        /// to avoid the overhead associated with mipmap generation.
        bool create_mipmaps = false;

        /// Anisotropy factor, if anisotropic filtering is supported.
        /// A value <= 1 means no anisotropic filtering.
        float anisotropy = 1;
    };

    // methods -------------------------------------------------------------------------------------------------------//
protected:
    /// Constructor.
    /// @param args Arguments.
    RenderTarget(Args&& args);

public:
    DISALLOW_COPY_AND_ASSIGN(RenderTarget)

    /// Factory.
    /// @param args Arguments.
    static RenderTargetPtr create(Args&& args);

    /// Binds the RenderTarget as a framebuffer.
    void bind_as_framebuffer();

    /// Binds the RenderTarget as a texture.
    /// @param slot     Texture slot to bind at.
    void bind_as_texture(uint slot);

private:
    /// Graphics context.
    GraphicsContext& m_context;

    /// Name of the RenderTarget, unique within the RenderManager.
    std::string m_name;

    /// Framebuffer to render into.
    FrameBufferPtr m_framebuffer;
};

} // namespace notf
