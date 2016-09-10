#pragma once

#include "core/components/shape_component.hpp"

namespace signal {

/// \brief Simple, Rectangular shape.
///
class RectShape : public ShapeComponent {

public: // methods
    /// \brief Returns the Shape's Axis Aligned Bounding Rect in screen coordinates.
    /// \param widget   Widget determining the shape.
    ///
    virtual Aabr get_screen_aabr(const Widget& widget) override;

    /// \brief Sets the minimal/maximal width/height of this Shape to its preferred width/height.
    void set_fixed();

    /// \brief Sets the minimal width of this Shape.
    /// The minimal width is clamped to a value <= preferred width.
    /// Minimal value is zero.
    void set_min_width(const Real width);

    /// \brief Sets the maximal width of this Shape.
    /// The maximal width is clamped to a value >= preferred width.
    /// Can be INFINITY.
    void set_max_width(const Real width);

    /// \brief Sets the minimal height of this Shape.
    /// The minimal height is clamped to a value <= preferred height.
    /// Minimal value is zero.
    void set_min_height(const Real height);

    /// \brief Sets the maximal height of this Shape.
    /// The maximal height is clamped to a value >= preferred height.
    /// Can be INFINITY.
    void set_max_height(const Real height);

protected: // methods
    /// \brief Value Constructor.
    /// Produces a RectShape of fixed size.
    explicit RectShape(const Real width, const Real height)
        : ShapeComponent()
    {
        set_horizontal_size({width});
        set_vertical_size({height});
    }
};

} // namespace signal
