#pragma once

#include <vector>

#include "common/forwards.hpp"
#include "common/size2.hpp"
#include "common/vector2.hpp"

namespace notf {

// ===================================================================================================================//

/// Plates are screen-axis-aligned quads that are drawn directly into the screen buffer by the RenderManager.
class Layer {

    // methods -------------------------------------------------------------------------------------------------------//
protected:
    /// Constructor.
    /// Constructs a full-screen, visible Layer.
    /// @param manager      RenderManager owning this Layer.
    /// @param producer     GraphicsProducer that renders into this Layer.
    Layer(RenderManagerPtr& manager, GraphicsProducerPtr producer);

public:
    DISALLOW_COPY_AND_ASSIGN(Layer)

    /// Factory.
    /// Constructs a full-screen, visible Layer.
    /// @param manager      RenderManager owning this Layer.
    /// @param producer     GraphicsProducer that renders into this Layer.
    static LayerPtr create(RenderManagerPtr& manager, GraphicsProducerPtr producer);

    /// Render the Layer with all of its effects.
    void render();

    /// Whether the Layer is visible or not.
    bool is_visible() const { return m_is_visible; }

    /// Whether the Layer is fullscreen or not.
    bool is_fullscreen() const { return m_is_fullscreen; }

    /// Size of this Layer in pixels.
    const Size2s& size() const { return m_size; }

    /// Position of this Layer relative to the origin.
    const Vector2s& position() const { return m_position; }

    /// Sets the Layer to be visible or not.
    void set_visible(const bool is_visible) { m_is_visible = is_visible; }

    /// Sets the Layer to either be rendered always fullscreen (no matter the resolution),
    /// or to respect its explicit size and position.
    void set_fullscreen(const bool is_fullscreen) { m_is_fullscreen = is_fullscreen; }

    /// Sets a new size for this Layer (but does not change its `fullscreen` state).
    void set_size(const Size2s size) { m_size = std::move(size); }

    /// Sets a new position for this Layer (but does not change its `fullscreen` state).
    void set_position(const Vector2s position) { m_position = std::move(position); }

    // fields --------------------------------------------------------------------------------------------------------//
private:
    /// RenderManager owning this Layer.
    RenderManager& m_render_manager;

    /// GraphicsProducer that renders into this Layer.
    GraphicsProducerPtr m_producer;

    /// Size of this Layer in pixels.
    Size2s m_size;

    /// Position of this Layer relative to the origin.
    Vector2s m_position;

    /// Layers can be set invisible in which case they are simply not drawn.
    bool m_is_visible;

    /// Layers can be rendered either fullscreen (no matter the resolution), or in an AABR with explicit size and
    /// position.
    bool m_is_fullscreen;
};

} // namespace notf
