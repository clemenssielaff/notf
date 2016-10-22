#pragma once

#include "common/real.hpp"

namespace notf {

/// \brief A Stretch includes a lower bound, a preferred size and an upper bound.
/// The constructor enforces that min <= preferred <= max and that min >= 0.
///
class Stretch {

public: // methods
    /// \brief Default Constructor.
    Stretch()
        : m_preferred(0)
        , m_min(0)
        , m_max(0)
    {
    }

    /// \brief Value Constructor.
    /// \param preferred    Preferred size in local units, is limited to values >= 0.
    /// \param min          (optional) Minimum size, is clamped to 0 <= value <= preferred, defaults to 'preferred'.
    /// \param max          (optional) Maximum size, is clamped to preferred <= value, can be INFINITY, defaults to 'preferred'.
    Stretch(const Real preferred, const Real min = NAN, const Real max = NAN)
        : m_preferred(is_valid(preferred) ? notf::max(preferred, Real(0)) : 0)
        , m_min(is_valid(min) ? notf::min(std::max(Real(0), min), m_preferred) : m_preferred)
        , m_max(is_valid(preferred) ? (is_nan(max) ? m_preferred : notf::max(max, m_preferred)) : 0)
    {
    }

    /// \brief Preferred size in local units, is >= 0.
    Real get_preferred() const { return m_preferred; }

    /// \brief Minimum size in local units, is 0 <= min <= preferred.
    Real get_min() const { return m_min; }

    /// \brief Maximum size in local units, is >= preferred.
    Real get_max() const { return m_max; }

    /// \brief Tests if this Stretch is a fixed size where all 3 values are the same.
    bool is_fixed() const { return approx(m_preferred, m_min) && approx(m_preferred, m_max); }

private: // fields
    /// \brief Preferred size in local units, is >= 0.
    Real m_preferred;

    /// \brief Minimum size in local units, is 0 <= min <= preferred.
    Real m_min;

    /// \brief Maximum size in local units, is >= preferred.
    Real m_max;
};

} // namespace notf
