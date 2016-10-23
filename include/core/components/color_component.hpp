#pragma once

#include "common/color.hpp"
#include "core/component.hpp"

namespace notf {

/// @brief Virtual base class for all Color Components.
///
class ColorComponent : public Component {

public: // methods
    /// @brief This Component's type.
    virtual KIND get_kind() const override { return KIND::COLOR; }

    /// @brief Returns a Color by index.
    ///
    /// Color components have a variable number of colors.
    /// If a component does not provide the requested index, the color with the last valid index is returned instead.
    ///
    /// @param index    Index of the requested color.
    ///
    /// @return The requested Color.
    virtual Color get_color(int index) const = 0;

protected: // methods
    /// @brief Default Constructor.
    explicit ColorComponent() = default;
};

} // namespace notf
