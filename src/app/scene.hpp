#pragma once

#include "app/forwards.hpp"
#include "common/size2.hpp"

NOTF_OPEN_NAMESPACE

// ===================================================================================================================//

class Scene {

    // methods -------------------------------------------------------------------------------------------------------//
protected:
    /// Constructor.
    Scene() = default;

public:
    NOTF_NO_COPY_OR_ASSIGN(Scene)

    /// Destructor.
    virtual ~Scene();

    /// Handles an untyped event.
    /// @returns True iff the event was handled.
    virtual bool handle_event(Event& event) = 0;

    /// Resizes the view on this Scene.
    /// @param size Size of the view in pixels.
    virtual void resize_view(Size2i size) = 0;
};

NOTF_CLOSE_NAMESPACE
