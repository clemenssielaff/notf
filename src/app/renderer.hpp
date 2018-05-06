#pragma once

#include <vector>

#include "app/forwards.hpp"
#include "common/pointer.hpp"
#include "common/exception.hpp"

NOTF_OPEN_NAMESPACE

// ===================================================================================================================//

/// Base class for Renderer.
class Renderer {

    // types ---------------------------------------------------------------------------------------------------------//
public:
    NOTF_ALLOW_ACCESS_TYPES(Layer, RenderTarget);

    /// Exception thrown when a Renderer requires a Scene to render, but wasn't passed on.
    NOTF_EXCEPTION_TYPE(no_scene);

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
    /// @param scene        Depending on the type of Renderer, it might require a Scene to render or not.
    ///                     A Renderer is free to ignore the argument if you pass one.
    /// @throws no_scene    If the Renderer requires a Scene argument, but none was passed.
    virtual void _render(risky_ptr<Scene*> scene) const = 0;
};

// ===================================================================================================================//

template<>
class Renderer::Access<Layer> {
    friend class Layer;

    /// Constructor.
    /// @param renderer     The renderer to access.
    Access(Renderer& renderer) : m_renderer(renderer) {}

    /// Renders the Renderer.
    /// @param scene        Depending on the type of Renderer, it might require a Scene to render or not.
    ///                     A Renderer is free to ignore the argument if you pass one.
    /// @throws no_scene    If the Renderer requires a Scene argument, but none was passed.
    void render(risky_ptr<Scene*> scene) { m_renderer._render(scene); }

    // fields -----------------------------------------------------------------
private:
    /// The Renderer to access.
    Renderer& m_renderer;
};

template<>
class Renderer::Access<RenderTarget> {
    friend class RenderTarget;

    /// Constructor.
    /// @param renderer     The Renderer to access.
    Access(Renderer& renderer) : m_renderer(renderer) {}

    /// Renders the Renderer.
    /// @throws no_scene    If the Renderer type requires a Scene argument.
    void render() { m_renderer._render(nullptr); }

    // fields -----------------------------------------------------------------
private:
    /// The Renderer to access.
    Renderer& m_renderer;
};

NOTF_CLOSE_NAMESPACE
