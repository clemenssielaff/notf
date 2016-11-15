#pragma once

#include "common/float_utils.hpp"

namespace notf {

/**
 * @brief The Claim of a Widget determines how much space it will receive in its parent Layout.
 *
 * Widget Claims are not changed by the Layout, only by the Widget or User.
 * If the parent Layout cannot accompany the Items minimal size, then it must simply overflow the parent Layout.
 * Also has a min and max ratio betweeen horizontal and vertical.
 * For example, a circular Item may have a size range from 1 - 10 both vertically and horizontally, but should only
 * expand in the ration 1:1, to stay circular.
 * Also, scale factors, both for vertical and horizontal expansion.
 * Linear factors work great if you want all to expand at the same time, but not if you want some to expand before
 * others.
 * For that, you also need a priority system, where widgets in priority two are expanded before priority one.
 * Reversly, widget in priority -1 are shrunk before priority 0 etc.
 */
class Claim {

public: // class
    /// @brief A Claim has two Directions: horizontal and vertical.
    /// Both need to enforce constraints but both Directions are largely independent.
    class Direction {

    public: // methods
        /// @brief Default Constructor.
        explicit Direction() = default;

        /// @brief Value Constructor.
        /// @param preferred    Preferred size in local units, is limited to values >= 0.
        /// @param min          (optional) Minimum size, is clamped to 0 <= value <= preferred, defaults to 'preferred'.
        /// @param max          (optional) Maximum size, is clamped to preferred <= value, can be INFINITY, defaults to 'preferred'.
        Direction(const float preferred, const float min = NAN, const float max = NAN)
            : m_preferred(is_valid(preferred) ? notf::max(preferred, 0.f) : 0.f)
            , m_min(is_valid(min) ? notf::min(std::max(0.f, min), m_preferred) : m_preferred)
            , m_max(is_valid(preferred) ? (is_nan(max) ? m_preferred : notf::max(max, m_preferred)) : 0.f)
            , m_scale_factor(1.f)
            , m_priority(0.f)
        {
        }

        /// @brief Preferred size in local units, is >= 0.
        float get_preferred() const { return m_preferred; }

        /// @brief Minimum size in local units, is 0 <= min <= preferred.
        float get_min() const { return m_min; }

        /// @brief Maximum size in local units, is >= preferred.
        float get_max() const { return m_max; }

        /// @brief Tests if this Stretch is a fixed size where all 3 values are the same.
        bool is_fixed() const { return approx(m_preferred, m_min) && approx(m_preferred, m_max); }

        /// @brief Returns the scale factor of the LayoutItem in this direction.
        float get_scale_factor() const { return m_scale_factor; }

        /// @brief Returns the scale priority of the LayoutItem in this direction.
        int get_priority() const { return m_priority; }

        /// @brief Sets a new minimal size, accomodates both the preferred and max size if necessary.
        /// @param min  Minimal size, must be 0 <= size < INFINITY.
        void set_min(const float min);

        /// @brief Sets a new minimal size, accomodates both the min and preferred size if necessary.
        /// @param max  Maximal size, must be 0 <= size <= INFINITY.
        void set_max(const float max);

        /// @brief Adds an offset to the min, max and preferred value.
        /// The offset can be negative.
        /// Fields are truncated to be >= 0, invalid values are ignored.
        /// Useful, for example, if you want to add a fixed "spacing" to the claim of a Layout.
        void add_offset(const float offset);

        /// @brief Sets a new preferred size, accomodates both the min and max size if necessary.
        /// @param preferred    Preferred size, must be 0 <= size < INFINITY.
        void set_preferred(const float preferred);

        /// @brief Sets a new scale factor.
        /// @param preferred    Preferred size, must be 0 <= size < INFINITY.
        void set_scale_factor(const float factor);

        /// @brief Sets a new scaling priority.
        /// @param priority Scaling priority.
        void set_priority(const int priority) { m_priority = priority; }

        /// @brief Assignment operator.
        Direction& operator=(const Direction& other)
        {
            m_preferred = other.m_preferred;
            m_min = other.m_min;
            m_max = other.m_max;
            m_scale_factor = other.m_scale_factor;
            m_priority = other.m_priority;
            return *this;
        }

        /// @brief Equality operator.
        bool operator==(const Direction& other) const
        {
            return (approx(m_preferred, other.m_preferred)
                    && approx(m_min, other.m_min)
                    && (approx(m_max, other.m_max) || (is_inf(m_max) && is_inf(other.m_max)))
                    && approx(m_scale_factor, other.m_scale_factor)
                    && m_priority == other.m_priority);
        }

        /// @brief Inequality operator.
        bool operator!=(const Direction& other) const
        {
            return !(*this == other);
        }

        /// @brief In-place addition operator for Directions.
        Direction& operator+=(const Direction& other)
        {
            m_preferred += other.m_preferred;
            m_min += other.m_min;
            m_max += other.m_max;
            m_scale_factor += other.m_scale_factor;
            m_priority = max(m_priority, other.m_priority);
            return *this;
        }

        /// @brief In-place max operator for Directions.
        Direction& maxed(const Direction& other)
        {
            m_preferred = max(m_preferred, other.m_preferred);
            m_min = max(m_min, other.m_min);
            m_max = max(m_max, other.m_max);
            m_scale_factor += max(m_scale_factor, other.m_scale_factor);
            m_priority = max(m_priority, other.m_priority);
            return *this;
        }

    private: // fields
        /// @brief Preferred size, is: min <= size <= max.
        float m_preferred = 0;

        /// @brief Minimal size, is: 0 <= size <= preferred.
        float m_min = 0;

        /// @brief Maximal size, is: preferred <= size <= INFINITY.
        float m_max = INFINITY;

        /// @brief Scale factor, 0 means no scaling, is: 0 <= factor < INFINITY
        float m_scale_factor = 1.;

        /// @brief Scaling priority, is INT_MIN <= priority <= INT_MAX.
        int m_priority = 0;
    };

private: // class
    /// @brief A height-for-width ratio constraint of the Claim.
    /// Is its own class so two Ratios can be properly added.
    class Ratio {

    public: // methods
        /// @brief Default Constructor.
        explicit Ratio() = default;

        /// @brief Value Constructor.
        /// Setting one or both values to zero, results in an invalid Ratio.
        /// @param height   Height in continuous units, is 0 < height < INFINITY
        /// @param width    Width in units, is 0 < width < INFINITY
        Ratio(const float height, const float width = 1)
            : m_width(height)
            , m_height(width)
        {
            if (!notf::is_valid(height) || !notf::is_valid(width) || height <= 0 || width <= 0) {
                m_width = 0;
                m_height = 0;
            }
        }

        /// @brief Tests if this Ratio is valid.
        bool is_valid() const { return !(approx(m_width, 0) || approx(m_height, 0)); }

        /// @brief Returns the height-for-width ratio, is 0 if invalid.
        float get_height_for_width() const
        {
            if (!is_valid()) {
                return 0;
            }
            return m_height / m_width;
        }

        /// @brief Equality operator.
        bool operator==(const Ratio& other) const
        {
            return (approx(m_width, other.m_width) && approx(m_height, other.m_height));
        }

        /// @brief Inequality operator.
        bool operator!=(const Ratio& other) const
        {
            return !(*this == other);
        }

        /// @brief In-place, horizontal addition operator for Directions.
        Ratio& add_horizontal(const Ratio& other)
        {
            m_width += other.m_width;
            m_height = max(m_height, other.m_height);
            return *this;
        }

        /// @brief In-place, vertical addition operator for Directions.
        Ratio& add_vertical(const Ratio& other)
        {
            m_width = max(m_width, other.m_width);
            m_height += other.m_height;
            return *this;
        }

    private: // fields
        /// @brief Width in discrete steps.
        float m_width;

        /// @brief Height in continuous units.
        float m_height;
    };

public: // methods
    /// @brief Default Constructor.
    explicit Claim() = default;

    /// @brief Copy Constructor.
    Claim(const Claim& other) = default;

    /// @brief Returns the horizontal part of this Claim.
    Direction& get_horizontal() { return m_horizontal; }

    /// @brief Returns the vertical part of this Claim.
    Direction& get_vertical() { return m_vertical; }

    /// Returns the min and max ratio constraints, 0 means no constraint, is: 0 <= min <= max < INFINITY
    std::pair<float, float> get_height_for_width() const
    {
        return {m_ratios.first.get_height_for_width(), m_ratios.second.get_height_for_width()};
    }

    /// @brief Sets the ratio constraint.
    /// @param ratio_min    Height for width (min/fixed value), is used as minimum value if the second parameter is set.
    /// @param ratio_max    Height for width (max value), 'ratio_min' is use by default.
    void set_height_for_width(const float ratio_min, const float ratio_max = NAN);

    /// @brief Assignment operator
    Claim& operator=(const Claim& other)
    {
        m_horizontal = other.m_horizontal;
        m_vertical = other.m_vertical;
        m_ratios = other.m_ratios;
        return *this;
    }

    /// @brief Equality operator.
    bool operator==(const Claim& other) const
    {
        return (m_horizontal == other.m_horizontal
                && m_vertical == other.m_vertical
                && m_ratios == other.m_ratios);
    }

    /// @brief Inequality operator.
    bool operator!=(const Claim& other) const
    {
        return !(*this == other);
    }

    /// @brief In-place, horizontal addition operator for Claims.
    Claim& add_horizontal(const Claim& other)
    {
        m_horizontal += other.m_horizontal;
        m_vertical.maxed(other.m_vertical);
        m_ratios = {
            m_ratios.first.add_horizontal(other.m_ratios.first),
            m_ratios.second.add_horizontal(other.m_ratios.second),
        };
        return *this;
    }

    /// @brief In-place, vertical addition operator for Claims.
    Claim& add_vertical(const Claim& other)
    {
        m_horizontal.maxed(other.m_horizontal);
        m_vertical += other.m_vertical;
        m_ratios = {
            m_ratios.first.add_vertical(other.m_ratios.first),
            m_ratios.second.add_vertical(other.m_ratios.second),
        };
        return *this;
    }

private: // members
    /// @brief The vertical part of this Claim.
    Direction m_horizontal;

    /// @brief The horizontal part of this Claim.
    Direction m_vertical;

    /// @brief Minimum and maximum ratio scaling constraint.
    std::pair<Ratio, Ratio> m_ratios;
};

} // namespace notf
