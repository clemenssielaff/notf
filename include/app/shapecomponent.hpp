#pragma once

#include "app/component.hpp"

namespace signal {

class ShapeComponent : public Component {
public: // methods
    explicit ShapeComponent();

    /// \brief This Component's type.
    virtual KIND get_kind() const override { return KIND::SHAPE; }
};

} // namespace signal
