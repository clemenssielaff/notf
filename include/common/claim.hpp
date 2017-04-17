#pragma once

#include <iosfwd>

#include "common/float.hpp"
#include "common/hash.hpp"

namespace notf {

/** The Claim of a Widget determines how much space it will receive in its parent Layout.
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
    /** A Claim has two Stretches, one in each direction: horizontal and vertical.
     * Both need to enforce constraints but both Stretches are largely independent.
     */
    class Stretch {

    public: // methods
        Stretch()
            : Stretch(0, 0, INFINITY) {}

        /**
         * @param preferred    Preferred size in local units, is limited to values >= 0.
         * @param min          (optional) Minimum size, is clamped to 0 <= value <= preferred, defaults to 'preferred'.
         * @param max          (optional) Maximum size, is clamped to preferred <= value, can be INFINITY, defaults to 'preferred'.
         */
        Stretch(const float preferred, const float min = NAN, const float max = NAN)
            : m_preferred(is_real(preferred) ? notf::max(preferred, 0.f) : 0.f)
            , m_min(is_real(min) ? notf::min(std::max(0.f, min), m_preferred) : m_preferred)
            , m_max(is_real(preferred) ? (is_nan(max) ? m_preferred : notf::max(max, m_preferred)) : 0.f)
            , m_scale_factor(1.f)
            , m_priority(0.f)
        {
        }

        /** Copy constructor. */
        Stretch(const Stretch& other)
            : m_preferred(other.m_preferred)
            , m_min(other.m_min)
            , m_max(other.m_max)
            , m_scale_factor(other.m_scale_factor)
            , m_priority(other.m_priority)
        {
        }

        /** Preferred size in local units, is >= 0. */
        float get_preferred() const { return m_preferred; }

        /** Minimum size in local units, is 0 <= min <= preferred. */
        float get_min() const { return m_min; }

        /** Maximum size in local units, is >= preferred. */
        float get_max() const { return m_max; }

        /** Tests if this Stretch is a fixed size where all 3 values are the same. */
        bool is_fixed() const { return m_preferred == approx(m_min) && m_preferred == approx(m_max); }

        /** Returns the scale factor. */
        float get_scale_factor() const { return m_scale_factor; }

        /** Returns the scale priority. */
        int get_priority() const { return m_priority; }

        /** Sets a new preferred size, accomodates both the min and max size if necessary.
         * @param preferred    Preferred size, must be 0 <= size < INFINITY.
         */
        void set_preferred(const float preferred);

        /** Sets a new minimal size, accomodates both the preferred and max size if necessary.
         * @param min  Minimal size, must be 0 <= size < INFINITY.
         */
        void set_min(const float min);

        /** Sets a new maximal size, accomodates both the min and preferred size if necessary.
         * @param max  Maximal size, must be 0 <= size <= INFINITY.
         */
        void set_max(const float max);

        /** Sets a new scale factor.
         * @param factor    Scale factor, must be 0 < factor < INFINITY.
         */
        void set_scale_factor(const float factor);

        /** Sets a new scaling priority. */
        void set_priority(const int priority) { m_priority = priority; }

        /** Sets a fixed size. */
        void set_fixed(const float size) { m_min = m_max = m_preferred = size; }

        /** Adds an offset to the min, max and preferred value.
         * The offset can be negative.
         * Fields are truncated to be >= 0, invalid values are ignored.
         * Useful, for example, if you want to add a fixed "spacing" to the claim of a Layout.
         */
        void add_offset(const float offset);

        Stretch& operator=(const Stretch& other)
        {
            m_preferred    = other.m_preferred;
            m_min          = other.m_min;
            m_max          = other.m_max;
            m_scale_factor = other.m_scale_factor;
            m_priority     = other.m_priority;
            return *this;
        }

        bool operator==(const Stretch& other) const
        {
            return (m_preferred == approx(other.m_preferred)
                    && m_min == approx(other.m_min)
                    && (m_max == approx(other.m_max) || (is_inf(m_max) && is_inf(other.m_max)))
                    && m_scale_factor == approx(other.m_scale_factor)
                    && m_priority == other.m_priority);
        }

        bool operator!=(const Stretch& other) const { return !(*this == other); }

        /** In-place addition operator. */
        Stretch& operator+=(const Stretch& other)
        {
            m_preferred += other.m_preferred;
            m_min += other.m_min;
            m_max += other.m_max;
            m_scale_factor += other.m_scale_factor;
            m_priority = max(m_priority, other.m_priority);
            return *this;
        }

        /** In-place max operator. */
        Stretch& maxed(const Stretch& other)
        {
            m_preferred    = max(m_preferred, other.m_preferred);
            m_min          = max(m_min, other.m_min);
            m_max          = max(m_max, other.m_max);
            m_scale_factor = max(m_scale_factor, other.m_scale_factor);
            m_priority     = max(m_priority, other.m_priority);
            return *this;
        }

    private: // fields
        /** Preferred size, is: min <= size <= max. */
        float m_preferred = 0;

        /** Minimal size, is: 0 <= size <= preferred. */
        float m_min = 0;

        /** Maximal size, is: preferred <= size <= INFINITY. */
        float m_max = INFINITY;

        /** Scale factor, 0 means no scaling, is: 0 <= factor < INFINITY */
        float m_scale_factor = 1.;

        /// @brief Scaling priority, is INT_MIN <= priority <= INT_MAX.
        int m_priority = 0;
    };

private: // class
    /** A height-for-width ratio constraint of the Claim.
     * Is its own class so two Ratios can be properly added.
     */
    class Ratio {

    public: // methods
        /** Default Constructor. */
        Ratio()
            : Ratio(0, 0) {}

        /** Setting one or both values to zero, results in an invalid Ratio.
         * @param width    Width in units, is 0 < width < INFINITY
         * @param height   Height in units, is 0 < height < INFINITY
         */
        Ratio(const float width, const float height = 1)
            : m_width(width)
            , m_height(height)
        {
            if (!notf::is_real(width) || !notf::is_real(height) || width <= 0 || height <= 0) {
                m_width  = 0;
                m_height = 0;
            }
        }

        /** Tests if this Ratio is valid. */
        bool is_valid() const { return !(m_width == approx(0) || m_height == approx(0)); }

        /** Returns the ratio, is 0 if invalid. */
        float get_width_to_height() const
        {
            if (!is_valid()) {
                return 0;
            }
            return m_width / m_height;
        }

        bool operator==(const Ratio& other) const
        {
            return (m_width == approx(other.m_width) && m_height == approx(other.m_height));
        }

        bool operator!=(const Ratio& other) const { return !(*this == other); }

        /** In-place, horizontal addition operator. */
        Ratio& add_horizontal(const Ratio& other)
        {
            m_width += other.m_width;
            m_height = max(m_height, other.m_height);
            return *this;
        }

        /** In-place, vertical addition operator. */
        Ratio& add_vertical(const Ratio& other)
        {
            m_width = max(m_width, other.m_width);
            m_height += other.m_height;
            return *this;
        }

    private: // fields
        /** Width. */
        float m_width;

        /** Height. */
        float m_height;
    };

public: // methods
    explicit Claim() = default;

    Claim(Claim::Stretch horizontal, Claim::Stretch vertical)
        : m_horizontal(std::move(horizontal)), m_vertical(std::move(vertical)), m_ratios() {}

    Claim(const Claim& other) = default;

    /** Returns the horizontal part of this Claim. */
    Stretch& get_horizontal() { return m_horizontal; }
    const Stretch& get_horizontal() const { return m_horizontal; }

    /** Returns the vertical part of this Claim. */
    Stretch& get_vertical() { return m_vertical; }
    const Stretch& get_vertical() const { return m_vertical; }

    /** Sets the horizontal Stretch of this Claim. */
    void set_horizontal(const Stretch& stretch) { m_horizontal = stretch; }

    /** Sets the vertical Stretch of this Claim. */
    void set_vertical(const Stretch& stretch) { m_vertical = stretch; }

    /** In-place, horizontal addition operator for Claims. */
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

    /** In-place, vertical addition operator for Claims. */
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

    /** Returns the min and max ratio constraints.
     * 0 means no constraint, is: 0 <= min <= max < INFINITY
     */
    std::pair<float, float> get_width_to_height() const
    {
        return {m_ratios.first.get_width_to_height(), m_ratios.second.get_width_to_height()};
    }

    /** Sets the ratio constraint.
     * @param ratio_min    Width to Height (min/fixed value), is used as minimum value if the second parameter is set.
     * @param ratio_max    Width to Height (max value), 'ratio_min' is use by default.
     */
    void set_width_to_height(const float ratio_min, const float ratio_max = NAN);

    Claim& operator=(const Claim& other)
    {
        m_horizontal = other.m_horizontal;
        m_vertical   = other.m_vertical;
        m_ratios     = other.m_ratios;
        return *this;
    }

    bool operator==(const Claim& other) const
    {
        return (m_horizontal == other.m_horizontal
                && m_vertical == other.m_vertical
                && m_ratios == other.m_ratios);
    }

    bool operator!=(const Claim& other) const { return !(*this == other); }

private: // members
    /** The vertical part of this Claim. */
    Stretch m_horizontal;

    /** The horizontal part of this Claim. */
    Stretch m_vertical;

    /** Minimum and maximum ratio scaling constraint. */
    std::pair<Ratio, Ratio> m_ratios;
};

/* Free Functions *****************************************************************************************************/

/** Prints the contents of a Claim::Stretch into a std::ostream.
 * @param out       Output stream, implicitly passed with the << operator.
 * @param stretch   Claim::Stretch to print.
 * @return          Output stream for further output.
 */
std::ostream& operator<<(std::ostream& out, const Claim::Stretch& stretch);

/** Prints the contents of a Claim into a std::ostream.
 * @param out   Output stream, implicitly passed with the << operator.
 * @param claim Claim to print.
 * @return      Output stream for further output.
 */
std::ostream& operator<<(std::ostream& out, const Claim& aabr);

} // namespace notf

/* std::hash **********************************************************************************************************/

namespace std {

/** std::hash specialization for notf::Claim::Stretch. */
template <>
struct hash<notf::Claim::Stretch> {
    size_t operator()(const notf::Claim::Stretch& stretch) const
    {
        return notf::hash(
            stretch.get_preferred(),
            stretch.get_min(),
            stretch.get_max(),
            stretch.get_scale_factor(),
            stretch.get_priority());
    }
};

/** std::hash specialization for notf::Claim. */
template <>
struct hash<notf::Claim> {
    size_t operator()(const notf::Claim& claim) const
    {
        const std::pair<float, float> ratio = claim.get_width_to_height();
        return notf::hash(
            claim.get_horizontal(),
            claim.get_horizontal(),
            ratio.first, ratio.second);
    }
};
}
