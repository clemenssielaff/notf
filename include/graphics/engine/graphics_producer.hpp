#pragma once

namespace notf {

// ===================================================================================================================//

/// Base class for GraphicsProducer.
struct GraphicsProducer {

    /// GraphicsProducer subclasses must identify themselves to the RenderManager, so it can try to minimize graphics
    /// state changes when rendering multiple GraphicsProducers in sequence.
    enum class Type {
        PLOTTER,
        PROCEDURAL,
    };

    /// Destructor.
    virtual ~GraphicsProducer();

    /// Unique type of this GraphicsProducer subclass.
    virtual Type render_type() const = 0;

    /// Render with the current graphics state.
    virtual void render() const = 0;

    /// Whether the GraphicsProducer is currently dirty or not.
    virtual bool is_dirty() const = 0;
};

} // namespace notf
