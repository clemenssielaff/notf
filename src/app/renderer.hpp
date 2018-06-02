#pragma once

#include <vector>

#include "app/forwards.hpp"
#include "common/exception.hpp"
#include "common/pointer.hpp"

NOTF_OPEN_NAMESPACE

// ================================================================================================================== //

/// Base class for Renderer.
class Renderer {

    // types -------------------------------------------------------------------------------------------------------- //
public:
    NOTF_ALLOW_ACCESS_TYPES(Layer, RenderTarget);

    // methods ------------------------------------------------------------------------------------------------------ //
public:
    /// Destructor.
    virtual ~Renderer();

private:
    /// Report all RenderTargets that this renderer depends on.
    /// The default implementation does nothing, it is the subclass' responsibility to add *all* of its dependencies.
    /// @param dependencies     [out] Dependencies to add yours to.
    virtual void _collect_dependencies(std::vector<RenderTarget*>& /*dependencies*/) const {}

    /// Subclass-defined implementation of the Renderer's rendering.
    /// @param scene    Scene to render.
    virtual void _render(valid_ptr<Scene*> scene) const = 0;
};

// ================================================================================================================== //

template<>
class Renderer::Access<Layer> {
    friend class Layer;

    /// Invokes the Renderer.
    /// @param scene    Scene to render.
    static void render(Renderer& renderer, valid_ptr<Scene*> scene) { renderer._render(std::move(scene)); }
};

template<>
class Renderer::Access<RenderTarget> {
    friend class RenderTarget;

    /// Invokes the Renderer.
    /// @param scene    Scene to render.
    static void render(Renderer& renderer, valid_ptr<Scene*> scene) { renderer._render(std::move(scene)); }
};

NOTF_CLOSE_NAMESPACE
