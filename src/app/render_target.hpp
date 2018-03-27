#pragma once

#include "app/forwards.hpp"
#include "common/exception.hpp"
#include "common/size2.hpp"

NOTF_OPEN_NAMESPACE

// ===================================================================================================================//

/// A RenderTarget is a 2D image of arbitrary size that is produced (and potentially consumed) by one or more
/// Renderers. Interally, they have a framebuffer with a single texture attached as color target. When one or
/// more of the target's Renderers are "dirty", the whole target has to be "cleaned" by evoking all of its
/// Renderers in order.
class RenderTarget {

    // types ---------------------------------------------------------------------------------------------------------//
public:
    /// RenderTarget arguments.
    struct Args {

        /// The Renderer that define the contents of the target.
        RendererPtr renderer;

        /// Size of the RenderTarget.
        Size2i size;

        /// Anisotropy factor, if anisotropic filtering is supported (only makes sense with `create_mipmaps = true`).
        /// A value <= 1 means no anisotropic filtering.
        float anisotropy = 1;

        /// Set to `true`, if this FrameBuffer has transparency.
        bool has_transparency = false;

        /// If you don't plan on transforming the RenderTarget before displaying it on screen, leave this set to `false`
        /// to avoid the overhead associated with mipmap generation.
        bool create_mipmaps = false;
    };

    // methods -------------------------------------------------------------------------------------------------------//
private:
    NOTF_ALLOW_MAKE_SMART_FROM_PRIVATE

    /// Constructor.
    /// @param context  The GraphicsContext containing the graphic objects.
    /// @param args     Arguments.
    RenderTarget(GraphicsContext& context, Args&& args);

public:
    NOTF_NO_COPY_OR_ASSIGN(RenderTarget)

    /// Factory.
    /// @param context      The GraphicsContext containing the graphic objects.
    /// @param args         Arguments.
    /// @throws value_error If `args` doesn't contain a Renderer.
    static RenderTargetPtr create(GraphicsContext& context, Args&& args)
    {
        if (!args.renderer) {
            notf_throw(value_error, "Cannot create a RenderTarget without a Renderer");
        }
        return NOTF_MAKE_UNIQUE_FROM_PRIVATE(RenderTarget, context, std::forward<Args>(args));
    }

    /// Destructor.
    ~RenderTarget();

    /// The FrameBuffer of this target.
    const FrameBufferPtr& framebuffer() const { return m_framebuffer; }

    /// Returns the texture of this target.
    const TexturePtr& texture() const;

    /// Renderer that draws into the target.
    const Renderer& renderer() const { return *m_renderer; }

    /// Whether the target is dirty or not.
    bool is_dirty() const { return m_is_dirty; }

    /// Evokes the Renderers, "cleaning" the target.
    /// If the target is clean to begin with, this does nothing.
    void clean();

    // fields --------------------------------------------------------------------------------------------------------//
private:
    /// The GraphicsContext containing the graphic objects.
    GraphicsContext& m_context;

    /// Framebuffer to render into.
    FrameBufferPtr m_framebuffer;

    /// Renderer that draws into the target.
    RendererPtr m_renderer;

    /// Whether the RenderTarget is currently dirty or not.
    bool m_is_dirty;
};

NOTF_CLOSE_NAMESPACE
