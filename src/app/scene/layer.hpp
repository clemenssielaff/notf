#pragma once

#include <vector>

#include "app/forwards.hpp"
#include "common/aabr.hpp"
#include "common/forwards.hpp"

NOTF_OPEN_NAMESPACE

// ===================================================================================================================//

/// Layers are screen-axis-aligned quads that are drawn directly into the screen buffer by the SceneManager.
/// The contents of a Layer are clipped to its area.
/// The Layer's GraphicsProducer can query the size of this area using GraphicsContext::render_area().size() when
/// rendered.
class Layer {

    // methods -------------------------------------------------------------------------------------------------------//
protected:
    /// Constructor.
    /// Constructs a full-screen, visible Layer.
    /// @param manager      SceneManager owning this Layer.
    /// @param producer     GraphicsProducer that renders into this Layer.
    Layer(SceneManagerPtr& manager, GraphicsProducerPtr producer);

public:
    NOTF_NO_COPY_OR_ASSIGN(Layer)

    /// Factory.
    /// Constructs a full-screen, visible Layer.
    /// @param manager      SceneManager owning this Layer.
    /// @param producer     GraphicsProducer that renders into this Layer.
    static LayerPtr create(SceneManagerPtr& manager, GraphicsProducerPtr producer);

    /// Render the Layer with all of its effects.
    void render();

    /// Whether the Layer is visible or not.
    bool is_visible() const { return m_is_visible; }

    /// Whether the Layer is fullscreen or not.
    bool is_fullscreen() const { return m_is_fullscreen; }

    /// Area of this Layer when not fullscreen.
    const Aabri& area() const { return m_area; }

    /// Sets the Layer to be visible or not.
    void set_visible(const bool is_visible) { m_is_visible = is_visible; }

    /// Sets the Layer to either be rendered always fullscreen (no matter the resolution),
    /// or to respect its explicit size and position.
    void set_fullscreen(const bool is_fullscreen) { m_is_fullscreen = is_fullscreen; }

    /// Sets a new are for this Layer to render tino (but does not change its `fullscreen` state).
    void set_area(Aabri area) { m_area = std::move(area); }

    // fields --------------------------------------------------------------------------------------------------------//
private:
    /// SceneManager owning this Layer.
    SceneManager& m_scene_manager;

    /// GraphicsProducer that renders into this Layer.
    GraphicsProducerPtr m_producer;

    /// Area of this Layer when not fullscreen.
    Aabri m_area;

    /// Layers can be set invisible in which case they are simply not drawn.
    bool m_is_visible;

    /// Layers can be rendered either fullscreen (no matter the resolution), or in an AABR with explicit size and
    /// position.
    bool m_is_fullscreen;
};

NOTF_CLOSE_NAMESPACE