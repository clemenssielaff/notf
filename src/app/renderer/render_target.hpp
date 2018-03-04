#pragma once

#include "app/forwards.hpp"
#include "common/forwards.hpp"
#include "common/id.hpp"
#include "common/size2.hpp"

NOTF_OPEN_NAMESPACE

// ===================================================================================================================//

/// RenderTarget id type.
using RenderTargetId = IdType<RenderTarget, size_t>;

// ===================================================================================================================//

/// A RenderTarget is a 2D image of arbitrary size that is produced (and potentially consumed) by one or more
/// GraphicsProducers. Interally, they have a framebuffer with a single texture attached as color target. When one or
/// more of the target's GraphicsProducers are "dirty", the whole target has to be "cleaned" by evoking all of its
/// GraphicsProducers in order.
class RenderTarget {

    // types ---------------------------------------------------------------------------------------------------------//
public:
    /// RenderTarget arguments.
    struct Args {
        /// Name of the RenderTarget, unique within the SceneManager.
        std::string name;

        /// Size of the RenderTarget.
        Size2i size;

        /// Set to `true`, if this FrameBuffer has transparency.
        bool has_transparency = false;

        /// If you don't plan on transforming the RenderTarget before displaying it on screen, leave this set to `false`
        /// to avoid the overhead associated with mipmap generation.
        bool create_mipmaps = false;

        /// Anisotropy factor, if anisotropic filtering is supported (only makes sense with `create_mipmaps = true`).
        /// A value <= 1 means no anisotropic filtering.
        float anisotropy = 1;

        /// The GraphicsProducer that define the contents of the target.
        GraphicsProducerPtr producer;
    };

    // methods -------------------------------------------------------------------------------------------------------//
protected:
    /// Constructor.
    /// @param manager  The SceneManager that the RenderTarget is registered with.
    /// @param args     Arguments.
    RenderTarget(SceneManager& scene_manager, Args&& args);

public:
    NOTF_NO_COPY_OR_ASSIGN(RenderTarget)

    /// Factory.
    /// @param manager  The SceneManager that the RenderTarget is registered with.
    /// @param args     Arguments.
    static RenderTargetPtr create(SceneManager& manager, Args&& args);

    /// Id of this RenderTarget.
    RenderTargetId id() const noexcept { return m_id; }

    /// Name of the RenderTarget, unique within the SceneManager.
    const std::string& name() const { return m_name; }

    /// The FrameBuffer of this target.
    const FrameBufferPtr& framebuffer() const { return m_framebuffer; }

    /// Returns the texture of this target.
    const TexturePtr& texture() const;

    /// Whether the target is dirty or not.
    bool is_dirty() const { return m_is_dirty; }

    /// Evokes the GraphicsProducers, "cleaning" the target.
    /// If the target is clean to begin with, this does nothing.
    void clean();

private:
    /// Generate the next available RenderTargetId.
    static RenderTargetId _next_id();

    // fields --------------------------------------------------------------------------------------------------------//
private:
    /// GraphicsProducer id.
    const RenderTargetId m_id;

    /// The GraphicsContext containing the graphic objects.
    GraphicsContext& m_context;

    /// Name of the RenderTarget, unique within the SceneManager.
    std::string m_name;

    /// Framebuffer to render into.
    FrameBufferPtr m_framebuffer;

    /// The GraphicsProducer that defines the contents of the target.
    GraphicsProducerPtr m_producer;

    /// Whether the RenderTarget is currently dirty or not.
    bool m_is_dirty;
};

NOTF_CLOSE_NAMESPACE