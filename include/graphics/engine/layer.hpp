#pragma once

#include <vector>

#include "common/forwards.hpp"

namespace notf {

// ===================================================================================================================//

/// Plates are full-screen quads that are the primary object managed by the RenderManager.
/// A Layer refers to a RenderTarget, a 2D image of arbitrary size. Each Layer also has an optional stack of PostEffects
/// (like blur) that are applied in order to the RenderTarget's texture.
class Layer {

    // TODO: Layer should also be fed using REnderers

    // methods -------------------------------------------------------------------------------------------------------//
public:
    /// Constructor.
    Layer() {}

    /// Render the Layer with all of its effects.
    void render();

    // fields --------------------------------------------------------------------------------------------------------//
private:
};

} // namespace notf
