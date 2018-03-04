#pragma once

#include "app/forwards.hpp"

NOTF_OPEN_NAMESPACE

// ===================================================================================================================//

class Scene {

    // methods -------------------------------------------------------------------------------------------------------//
protected:
    /// Constructor.
    Scene() = default;

public:
    NOTF_NO_COPY_OR_ASSIGN(Scene)

    /// Factory.
    static ScenePtr create();
};

NOTF_CLOSE_NAMESPACE
