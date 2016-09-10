#pragma once

#include "common/aabr.hpp"
#include "common/real.hpp"
#include "core/component.hpp"

namespace signal {

class Widget;

/// \brief Shapes have 2 SizeRanges, one for each axis.
/// A SizeRange includes a lower bound, a preferred size and an upper bound.
/// The constructor enforces that min <= preferred <= max and that min >= 0.
///
struct SizeRange {

    /// \brief Preferred size in local units, is >= 0.
    const Real preferred;

    /// \brief Minimum size in local units, is 0 <= min <= preferred.
    const Real min;

    /// \brief Maximum size in local units, is >= preferred.
    const Real max;

    /// \brief Default Constructor.
    SizeRange()
        : preferred(0)
        , min(0)
        , max(0)
    {
    }

    /// \brief Value Constructor.
    /// \param preferred    Preferred size in local units, is limited to values >= 0.
    /// \param min          (optional) Minimum size, is clamped to 0 <= value <= preferred, defaults to 'preferred'.
    /// \param max          (optional) Maximum size, is clamped to preferred <= value, can be INFINITY, defaults to 'preferred'.
    SizeRange(const Real preferred, const Real min = NAN, const Real max = NAN)
        : preferred(is_valid(preferred) ? signal::max(preferred, Real(0)) : 0)
        , min(is_valid(min) ? signal::min(std::max(Real(0), min), this->preferred) : this->preferred)
        , max(is_valid(preferred) ? (is_nan(max) ? this->preferred : signal::max(max, this->preferred)) : 0)
    {
    }

    /// \brief Swap implementation.
    /// Since all fields of the SizeRange class are constant, we cannot assign to them.
    /// We can however safely swap the entire object.
    friend void swap(SizeRange& first, SizeRange& second) noexcept
    {
        using std::swap;
        swap(first.preferred, second.preferred);
        swap(first.min, second.min);
        swap(first.max, second.max);
    }

    /// \brief Tests if this SizeRange is a fixed size where all 3 values are the same.
    bool is_fixed() const { return approx(preferred, min) && approx(preferred, max); }
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/// \brief Virtual base class for all Shape Components.
///
class ShapeComponent : public Component {

public: // methods
    /// \brief This Component's type.
    virtual KIND get_kind() const override { return KIND::SHAPE; }

    /// \brief Returns the Shape's Axis Aligned Bounding Rect in screen coordinates.
    /// \param widget   Widget determining the shape.
    virtual Aabr get_screen_aabr(const Widget& widget) = 0;

    /// \brief Returns the vertical size range of this Shape.
    const SizeRange& get_vertical_size() const { return m_vertical_size; }

    /// \brief Returns the horizontal size range of this Shape.
    const SizeRange& get_horizontal_size() const { return m_horizontal_size; }

protected: // methods
    /// \brief Default Constructor.
    explicit ShapeComponent() = default;

    /// \brief Sets the vertical size range of this Shape.
    void set_vertical_size(SizeRange size) { swap(m_vertical_size, size); }

    /// \brief Sets the horizontal size range of this Shape.
    void set_horizontal_size(SizeRange size) { swap(m_horizontal_size, size); }

private: // fields
    /// \brief Vertical size range.
    SizeRange m_vertical_size;

    /// \brief Horizontal size range.
    SizeRange m_horizontal_size;
};

} // namespace signal
