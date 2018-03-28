#pragma once

#include <vector>

#include "app/forwards.hpp"

NOTF_OPEN_NAMESPACE

// ===================================================================================================================//

/// Base class for Renderer.
class Renderer {

    // types ---------------------------------------------------------------------------------------------------------//
public:
    NOTF_ACCESS_TYPES(Layer, RenderTarget)

    // methods -------------------------------------------------------------------------------------------------------//
public:
    /// Destructor.
    virtual ~Renderer();

private:
    /// Report all RenderTargets that this renderer depends on.
    /// The default implementation does nothing, it is the subclass' responsibility to add *all* of its dependencies.
    /// @param dependencies     [out] Dependencies to add yours to.
    virtual void _collect_dependencies(std::vector<RenderTarget*>& /*dependencies*/) const {}

    /// Subclass-defined implementation of the Renderer's rendering.
    virtual void _render() const = 0;
};

// ===================================================================================================================//

template<>
class Renderer::Access<Layer> {
    friend class Layer;

    /// Constructor.
    /// @param renderer     The renderer to access.
    Access(Renderer& renderer) : m_renderer(renderer) {}

    /// Renders the Renderer, if it is dirty.
    void render() { m_renderer._render(); }

    /// The Renderer to access.
    Renderer& m_renderer;
};

template<>
class Renderer::Access<RenderTarget> {
    friend class RenderTarget;

    /// Constructor.
    /// @param renderer     The Renderer to access.
    Access(Renderer& renderer) : m_renderer(renderer) {}

    /// Renders the Renderer, if it is dirty.
    void render() { m_renderer._render(); }

    /// The Renderer to access.
    Renderer& m_renderer;
};

NOTF_CLOSE_NAMESPACE
