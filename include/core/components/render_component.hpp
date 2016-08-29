#pragma once

#include "core/component.hpp"

namespace signal {

class Widget;

/// \brief Virtual base class for all Render Components.
///
class RenderComponent : public Component {

public: // methods
    /// \brief This Component's type.
    virtual KIND get_kind() const override { return KIND::RENDER; }

    /// \brief Renders the given Widget.
    /// \param widget   Widget to render.
    ///
    virtual void render(Widget& widget) = 0;

protected: // methods
    /// \brief Default Constructor.
    explicit RenderComponent() = default;
};

} // namespace signal
