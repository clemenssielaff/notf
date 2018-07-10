#pragma once

#include <vector>

#include "app/forwards.hpp"
#include "common/pointer.hpp"
#include "common/size2.hpp"

NOTF_OPEN_NAMESPACE

// ================================================================================================================== //

/// Base class for Visualizer.
class Visualizer {

    // methods ------------------------------------------------------------------------------------------------------ //
public:
    /// Destructor.
    virtual ~Visualizer();

    /// Subclass-defined visualization implementation.
    /// @param scene    Scene to visualize.
    virtual void visualize(valid_ptr<Scene*> scene) const = 0;

private:
    /// Report all Plates that this Visualizer depends on.
    /// The default implementation does nothing, it is the subclass' responsibility to add *all* of its dependencies.
    /// @param dependencies     [out] Dependencies to add yours to.
    virtual void _collect_dependencies(std::vector<Plate*>& /*dependencies*/) const {}
};

// ================================================================================================================== //

/// A Plate is a 2D image of arbitrary size that is produced (and potentially consumed) by one or more Visualizers.
/// Interally, they have a framebuffer with a single texture attached as color target. When one or more of the target's
/// Visualizers are "dirty", the whole target has to be "cleaned" by evoking all of its Visualizers in order.
class Plate {

    // types -------------------------------------------------------------------------------------------------------- //
public:
    /// Plate arguments.
    struct Args {

        /// The Scene to visualize.
        ScenePtr scene;

        /// The Visualizer that define the contents of the target.
        VisualizerPtr visualizer;

        /// Size of the Plate.
        Size2i size;

        /// Anisotropy factor, if anisotropic filtering is supported (only makes sense with `create_mipmaps = true`).
        /// A value <= 1 means no anisotropic filtering.
        float anisotropy = 1;

        /// Set to `true`, if this FrameBuffer has transparency.
        bool has_transparency = false;

        /// If you don't plan on transforming the Plate before displaying it on screen, leave this set to `false`
        /// to avoid the overhead associated with mipmap generation.
        bool create_mipmaps = false;
    };

    // methods ------------------------------------------------------------------------------------------------------ //
private:
    NOTF_ALLOW_MAKE_SMART_FROM_PRIVATE;

    /// Constructor.
    /// @param context  The GraphicsContext containing the graphic objects.
    /// @param args     Arguments.
    Plate(GraphicsContext& context, Args&& args);

public:
    NOTF_NO_COPY_OR_ASSIGN(Plate);

    /// Factory.
    /// @param context      The GraphicsContext containing the graphic objects.
    /// @param args         Arguments.
    /// @throws value_error If `args` doesn't contain a Visualizer.
    static PlatePtr create(GraphicsContext& context, Args&& args)
    {
        if (!args.visualizer) {
            NOTF_THROW(value_error, "Cannot create a Plate without a Visualizer");
        }
        if (!args.scene) {
            NOTF_THROW(value_error, "Cannot create a Plate without a Scene to visualize");
        }
        return NOTF_MAKE_SHARED_FROM_PRIVATE(Plate, context, std::forward<Args>(args));
    }

    /// Destructor.
    ~Plate();

    /// The FrameBuffer of this target.
    const FrameBufferPtr& framebuffer() const { return m_framebuffer; }

    /// Returns the texture of this target.
    const TexturePtr& texture() const;

    /// Visualizer that draws into the target.
    const Visualizer& visualizer() const { return *m_visualizer; }

    /// Whether the target is dirty or not.
    bool is_dirty() const { return m_is_dirty; }

    /// Evokes the Visualizer, "cleaning" the target.
    /// If the target is clean to begin with, this does nothing.
    void clean();

    // fields ------------------------------------------------------------------------------------------------------- //
private:
    /// Framebuffer to render into.
    FrameBufferPtr m_framebuffer;

    /// The Scene to visualize.
    ScenePtr m_scene;

    /// Visualizer that draws into the target.
    VisualizerPtr m_visualizer;

    /// Whether the Plate is currently dirty or not.
    bool m_is_dirty = true;
};

NOTF_CLOSE_NAMESPACE
