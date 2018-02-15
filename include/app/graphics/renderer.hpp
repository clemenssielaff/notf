#pragma once

namespace notf {

// ===================================================================================================================//

/// Base class for Renderers.
struct Renderer {

    /// Renderer subclasses must identify themselves to the RenderManager, so it can try to minimize graphics state
    /// changes when rendering multiple Renderers in sequence.
    enum class Type {
        PLOTTER,
        PROCEDURAL,
    };

    /// Destructor.
    virtual ~Renderer();

    /// Unique type of this Renderer subclass.
    virtual Type render_type() const = 0;

    /// Render with the current graphics state.
    virtual void render() const = 0;

    /// Whether the Renderer is currently dirty or not.
    virtual bool is_dirty() const = 0;
};

} // namespace notf
