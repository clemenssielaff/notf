#pragma once

#include <vector>

#include "common/forwards.hpp"

namespace notf {

// ===================================================================================================================//

/// Plates are full-screen quads that are the primary object managed by the RenderManager.
/// A plate refers to a RenderTarget, a 2D image of arbitrary size. Each plate also has an optional stack of PostEffects
/// (like blur) that are applied in order to the RenderTarget's texture.
class Plate {

    // TODO: Plate should also be fed using REnderers

    // methods -------------------------------------------------------------------------------------------------------//
public:
    /// Constructor.
    /// @param render_target    Render Target defining the content of the Plate.
    Plate(RenderTargetPtr render_target) : m_render_target(std::move(render_target)), m_effects() {}

    /// Render the plate with all of its effects.
    void render();

    // fields --------------------------------------------------------------------------------------------------------//
private:
    /// The RenderTarget used to render this Plate into and read it from.
    RenderTargetPtr m_render_target;

    /// Effects are applied in order whenever the Plate is rendered.
    std::vector<PostEffectPtr> m_effects;
};

} // namespace notf