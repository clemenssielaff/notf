#pragma once

#include "common/aabr.hpp"
#include "core/component.hpp"

namespace signal {

class Widget;

/// \brief Virtual ase class for all Shape Components.
///
class ShapeComponent : public Component {

public: // methods
    /// \brief This Component's type.
    virtual KIND get_kind() const override { return KIND::SHAPE; }

    /// \brief Returns the Aabr of the shape of the given Widget.
    /// \param widget   Widget determining the shape.
    ///
    virtual Aabr bounding_rect(Widget& widget) = 0;

protected: // methods
    /// \brief Default Constructor.
    explicit ShapeComponent() = default;
};

} // namespace signal
