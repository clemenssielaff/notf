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
class SizeRange {

public: // methods
    /// \brief Default Constructor.
    SizeRange()
        : m_preferred(0)
        , m_min(0)
        , m_max(0)
    {
    }

    /// \brief Value Constructor.
    /// \param preferred    Preferred size in local units, is limited to values >= 0.
    /// \param min          (optional) Minimum size, is clamped to 0 <= value <= preferred, defaults to 'preferred'.
    /// \param max          (optional) Maximum size, is clamped to preferred <= value, can be INFINITY, defaults to 'preferred'.
    SizeRange(const Real preferred, const Real min = NAN, const Real max = NAN)
        : m_preferred(is_valid(preferred) ? signal::max(preferred, Real(0)) : 0)
        , m_min(is_valid(min) ? signal::min(std::max(Real(0), min), m_preferred) : m_preferred)
        , m_max(is_valid(preferred) ? (is_nan(max) ? m_preferred : signal::max(max, m_preferred)) : 0)
    {
    }

    /// \brief Preferred size in local units, is >= 0.
    Real get_preferred() const { return m_preferred; }

    /// \brief Minimum size in local units, is 0 <= min <= preferred.
    Real get_min() const { return m_min; }

    /// \brief Maximum size in local units, is >= preferred.
    Real get_max() const { return m_max; }

    /// \brief Tests if this SizeRange is a fixed size where all 3 values are the same.
    bool is_fixed() const { return approx(m_preferred, m_min) && approx(m_preferred, m_max); }

private: // fields
    /// \brief Preferred size in local units, is >= 0.
    Real m_preferred;

    /// \brief Minimum size in local units, is 0 <= min <= preferred.
    Real m_min;

    /// \brief Maximum size in local units, is >= preferred.
    Real m_max;
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

public: // signals
    /// \brief Emitted, when the horizontal size of this Shape changed.
    /// \param New size range.
    Signal<const SizeRange&> horizontal_size_changed;

    /// \brief Emitted, when the vertical size of this Shape changed.
    /// \param New size range.
    Signal<const SizeRange&> vertical_size_changed;

protected: // methods
    /// \brief Default Constructor.
    explicit ShapeComponent() = default;

    /// \brief Sets the horizontal size range of this Shape.
    void set_horizontal_size(const SizeRange& size)
    {
        m_horizontal_size = size;
        horizontal_size_changed(m_horizontal_size);
    }

    /// \brief Sets the vertical size range of this Shape.
    void set_vertical_size(const SizeRange& size)
    {
        m_vertical_size = size;
        vertical_size_changed(m_vertical_size);
    }

private: // fields
    /// \brief Vertical size range.
    SizeRange m_vertical_size;

    /// \brief Horizontal size range.
    SizeRange m_horizontal_size;
};

} // namespace signal
