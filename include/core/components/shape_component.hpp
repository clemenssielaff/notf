#pragma once

#include "common/aabr.hpp"
#include "core/component.hpp"

namespace signal {

class Widget;

/// \brief Virtual base class for all Shape Components.
///
class ShapeComponent : public Component {

public: // methods
    /// \brief This Component's type.
    virtual KIND get_kind() const override { return KIND::SHAPE; }

    /// \brief Returns the Shape's Axis Aligned Bounding Rect.
    /// \param widget   Widget determining the shape.
    ///
    virtual Aabr get_aabr(const Widget& widget) = 0;

protected: // methods
    /// \brief Default Constructor.
    explicit ShapeComponent() = default;
};

} // namespace signal
