#pragma once

#include "core/component.hpp"

namespace notf {

class Widget;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/// @brief Virtual base class for all Shape Components.
///
class ShapeComponent : public Component {

public: // methods
    /// @brief This Component's type.
    virtual KIND get_kind() const override { return KIND::SHAPE; }

public: // signals

protected: // methods
    /// @brief Default Constructor.
    explicit ShapeComponent() = default;

private: // fields

};

} // namespace notf
