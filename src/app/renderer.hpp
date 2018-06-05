#pragma once

#include <vector>

#include "app/forwards.hpp"
#include "common/exception.hpp"
#include "common/pointer.hpp"

NOTF_OPEN_NAMESPACE

namespace access { // forwards
template<class>
class _Renderer;
} // namespace access

// ================================================================================================================== //

/// Base class for Renderer.
class Renderer {

    friend class access::_Renderer<Layer>;
    friend class access::_Renderer<RenderTarget>;

    // types -------------------------------------------------------------------------------------------------------- //
public:
    /// Access types.
    template<class T>
    using Access = access::_Renderer<T>;

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
class access::_Renderer<Layer> {
    friend class notf::Layer;

    /// Invokes the Renderer.
    /// @param scene    Scene to render.
    static void render(Renderer& renderer, valid_ptr<Scene*> scene) { renderer._render(std::move(scene)); }
};

template<>
class access::_Renderer<RenderTarget> {
    friend class notf::RenderTarget;

    /// Invokes the Renderer.
    /// @param scene    Scene to render.
    static void render(Renderer& renderer, valid_ptr<Scene*> scene) { renderer._render(std::move(scene)); }
};

NOTF_CLOSE_NAMESPACE
