#pragma once

#include "app/forwards.hpp"
#include "common/aabr.hpp"
#include "common/pointer.hpp"

NOTF_OPEN_NAMESPACE

// ================================================================================================================== //

/// Layers are screen-axis-aligned quads that are drawn directly into the screen buffer by the SceneGraph.
/// The contents of a Layer are clipped to its area.
/// The Layer's Renderer can query the size of this area using GraphicsContext::render_area().size() when
/// rendered.
class Layer {

    // methods ------------------------------------------------------------------------------------------------------ //
private:
    NOTF_ALLOW_MAKE_SMART_FROM_PRIVATE;

    /// Constructor.
    /// Constructs a full-screen, visible Layer.
    /// @param window       Window containing this Layer.
    /// @param renderer     Renderer that renders the Scene into this Layer.
    /// @param scene        Scene displayed in this Layer.
    Layer(Window& window, valid_ptr<RendererPtr> renderer, valid_ptr<ScenePtr> scene);

public:
    NOTF_NO_COPY_OR_ASSIGN(Layer);

    /// Factory.
    /// Constructs a full-screen, visible Layer.
    /// @param window       Window containing this Layer.
    /// @param renderer     Renderer that renders the Scene into this Layer.
    /// @param scene        Scene displayed in this Layer.
    static LayerPtr create(Window& window, valid_ptr<RendererPtr> renderer, valid_ptr<ScenePtr> scene);

    /// Destructor.
    ~Layer();

    /// Whether the Layer is visible or not.
    bool is_visible() const { return m_is_visible; }

    /// Whether the Layer is active or not.
    bool is_active() const { return m_is_active; }

    /// Whether the Layer is fullscreen or not.
    bool is_fullscreen() const { return m_is_fullscreen; }

    /// Area of this Layer when not fullscreen.
    const Aabri& area() const { return m_area; }

    /// The Scene displayed in this Layer, might be empty.
    Scene& scene() { return *raw_pointer(m_scene); }

    /// Invisible Layers are not drawn on screen.
    /// Note that this method also changes the `active` state of the Layer to the visibility state.
    /// If you want a hidden/active or visible/inactive combo, call `set_active` after this method.
    void set_visible(const bool is_visible)
    {
        m_is_visible = is_visible;
        m_is_active = is_visible;
    }

    /// Inactive Layers do not participate in event propagation.
    void set_active(const bool is_active) { m_is_active = is_active; }

    /// Sets the Layer to either be rendered always fullscreen (no matter the resolution),
    /// or to respect its explicit size and position.
    void set_fullscreen(const bool is_fullscreen) { m_is_fullscreen = is_fullscreen; }

    /// Sets a new are for this Layer to render into (but does not change its `fullscreen` state).
    void set_area(Aabri area) { m_area = area; }

    /// Render the Layer.
    void render();

    // fields ------------------------------------------------------------------------------------------------------- //
private:
    /// Window containing this Layer.
    Window& m_window;

    /// The Scene displayed in this Layer.
    valid_ptr<ScenePtr> m_scene;

    /// Renderer that renders the Scene into this Layer.
    valid_ptr<RendererPtr> m_renderer;

    /// Area of this Layer when not fullscreen.
    Aabri m_area = Aabri::zero();

    /// Layers can be set invisible in which case they are simply not drawn.
    bool m_is_visible = true;

    /// Layers can be active (the default) or inactive, in which case they do not participate in the event propagation.
    bool m_is_active = true;

    /// Layers can be rendered either fullscreen (no matter the resolution), or in an AABR with explicit size and
    /// position.
    bool m_is_fullscreen = true;
};

NOTF_CLOSE_NAMESPACE
