#pragma once

#include <vector>

#include "common/forwards.hpp"

namespace notf {

// ===================================================================================================================//

// The Render manager has a STATE that defines how to render a frame.
class RenderManager {
    // types ---------------------------------------------------------------------------------------------------------//
public:
    /// Complete state of the Render Buffer.
    struct State {
        std::vector<LayerPtr> layers;
    };

    // fields --------------------------------------------------------------------------------------------------------//
private:

};

} // namespace notf
