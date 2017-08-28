#pragma once

#include <iosfwd>

#include "common/size2.hpp"

namespace notf {

/**********************************************************************************************************************/

/** Every ScreenItem has a Claim that determines how much space is alloted for it in its parent Layout.
 * The user can declare Claims manually for both Widgets and Layouts, although Layouts usually have a mechanism to
 * calculate their own Claim based on the combined Claims of their children.
 * A Claim is made up of serveral parts:
 *
 * Stretches
 * ---------
 * A Claim has 2 `Stretch` fields, one for its horizontal and one for its vertical extension.
 * Each Stretch consists of a *minimum* value, a *maximum* and a preferred value.
 * Usually, the ScreenItem assumes its *preferred* size first and is then regulated up or down, depending on how much
 * space is left in its parent Layout.
 *
 * The *Stretch factor* of a Strech determines, how fast a ScreenItem grows in relation to its siblings.
 * Two ScreenItems with Stretch factors of 1 each, will grow at the same rate when more space becomes available.
 * If one of them had a Stretch factor of 2, it would grow twice as fast as the other, until it reaches its maximum.
 * Conversely, a Stretch factor of 0.5 would make it grow only half as fast.
 * Strecth factors have to be larger than zero, assigning a stretch factor of <= 0 will cause a warning and the factor
 * will be clamped to a value > 0.
 *
 * The *priority* of a Stretch comes into play, when you want one ScreenItem to fully expand before any others are even
 * considered.
 * If you have 3 ScreenItems A, B and C and C has a priority of 1, while A and B have a priority of 0, then C will take
 * up all available space without giving any to A and B.
 * Only after C has reached its maximum size is the additional space distributed to A and B (using their individual
 * Stretch factors).
 * Conversely, if the available space should shrink then A and B are the first ones to give up their additional space.
 * C will only shrink after A and B have both reached their minimum size.
 *
 * You can modify the two Stretches of a Claim individually, or set them both using the Claim's functions.
 */
class Claim {

    //*****************************************************************************************************************/
public: // type
    class Stretch {
    public: // methods
        /** Default Constructor. */
        Stretch()
            : Stretch(0, 0, INFINITY) {}

        /** Value Constructor.
         * @param preferred    Preferred size in local units, is limited to values >= 0.
         * @param min          Minimum size, is clamped to 0 <= value <= preferred, defaults to 'preferred'.
         * @param max          Maximum size, is clamped to preferred <= value, can be INFINITY, defaults to 'preferred'.
         */
        Stretch(const float preferred, const float min = NAN, const float max = NAN)
            : m_preferred(is_real(preferred) ? notf::max(preferred, 0) : 0.f)
            , m_min(is_real(min) ? notf::min(notf::max(0, min), m_preferred) : m_preferred)
            , m_max(is_real(preferred) ? (is_nan(max) ? m_preferred : notf::max(max, m_preferred)) : 0.f)
            , m_scale_factor(1.f)
            , m_priority(0)
        {
        }

        /** Copy constructor. */
        Stretch(const Stretch& other) = default;

        /** Preferred size in local units, is >= 0. */
        float get_preferred() const { return m_preferred; }

        /** Minimum size in local units, is 0 <= min <= preferred. */
        float get_min() const { return m_min; }

        /** Maximum size in local units, is >= preferred. */
        float get_max() const { return m_max; }

        /** Tests if this Stretch is a fixed size where all 3 values are the same. */
        bool is_fixed() const
        {
            return abs(m_preferred - m_min) < precision_high<float>()
                && abs(m_preferred - m_max) < precision_high<float>();
        }

        /** Returns the scale factor. */
        float get_scale_factor() const { return m_scale_factor; }

        /** Returns the scale priority. */
        int get_priority() const { return m_priority; }

        /** Sets a new minimum size, accomodates both the preferred and max size if necessary.
         * @param min  Minimum size, must be 0 <= size < INFINITY.
         */
        void set_min(const float min);

        /** Sets a new preferred size, accomodates both the min and max size if necessary.
         * @param preferred    Preferred size, must be 0 <= size < INFINITY.
         */
        void set_preferred(const float preferred);

        /** Sets a new maximum size, accomodates both the min and preferred size if necessary.
         * @param max  Maximum size, must be 0 <= size <= INFINITY.
         */
        void set_max(const float max);

        /** Sets a new scale factor.
         * @param factor    Scale factor, is clamped to 0 < factor < INFINITY.
         */
        void set_scale_factor(const float factor);

        /** Sets a new scaling priority (0 = default). */
        void set_priority(const int priority) { m_priority = priority; }

        /** Sets a fixed size. */
        void set_fixed(const float size) { m_min = m_max = m_preferred = size; }

        /** Adds an offset to the min, max and preferred value.
        * Useful, for example, if you want to add a fixed "spacing" to the Claim of a Layout.
         * The offset can be negative.
         * Fields are truncated to be >= 0, invalid values are ignored.
         */
        void grow_by(const float offset);

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

        /** Assignment operator. */
        Stretch& operator=(const Stretch& other)
        {
            m_preferred    = other.m_preferred;
            m_min          = other.m_min;
            m_max          = other.m_max;
            m_scale_factor = other.m_scale_factor;
            m_priority     = other.m_priority;
            return *this;
        }

        /** Equality comparison operator. */
        bool operator==(const Stretch& other) const
        {
            return (abs(m_preferred - other.m_preferred) < precision_high<float>()
                    && abs(m_min - other.m_min) < precision_high<float>()
                    && (abs(m_max - other.m_max) < precision_high<float>() || (is_inf(m_max) && is_inf(other.m_max)))
                    && abs(m_scale_factor - other.m_scale_factor) < precision_high<float>()
                    && m_priority == other.m_priority);
        }

        /** Inequality comparison operator. */
        bool operator!=(const Stretch& other) const { return !(*this == other); }

        /** In-place addition operator. */
        Stretch& operator+=(const Stretch& other)
        {
            m_preferred += other.m_preferred;
            m_min += other.m_min;
            m_max += other.m_max;
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

    //*****************************************************************************************************************/
private: // class
    /** A height-for-width ratio constraint of the Claim.
     * Is its own class so two Ratios can be properly added.
     * A value of zero means no Ratio constraint.
     */
    class Ratio {

    public: // methods
        /** Default Constructor. */
        Ratio()
            : Ratio(0, 0) {}

        /** Value Constructor.
         * Setting one or both values to zero, results in an invalid Ratio.
         * @param width    Width in units, is 0 < width < INFINITY
         * @param height   Height in units, is 0 < height < INFINITY
         */
        Ratio(const float width, const float height = 1)
            : m_width(width)
            , m_height(height)
        {
            if (!is_real(width) || !is_real(height) || width <= 0 || height <= 0) {
                m_width  = 0;
                m_height = 0;
            }
        }

        /** Tests if this Ratio is valid. */
        bool is_valid() const { return m_width > precision_high<float>() && m_height > precision_high<float>(); }

        /** Returns the ratio, is 0 if invalid. */
        float get_height_for_width() const
        {
            if (!is_valid()) {
                return 0;
            }
            return m_height / m_width;
        }

        bool operator==(const Ratio& other) const
        {
            return (abs(m_width - other.m_width) < precision_high<float>())
                && (abs(m_height - other.m_height) < precision_high<float>());
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

    //*****************************************************************************************************************/

public: // static methods
    /** Returns a Claim with fixed height and width. */
    static Claim fixed(float width, float height);
    static Claim fixed(const Size2f& size) { return fixed(size.width, size.height); }

    /** Returns a Claim with all limits set to zero. */
    static Claim zero();

public: // methods
    /** Default Constructor. */
    Claim()
        : m_horizontal(), m_vertical(), m_ratios() {}

    /** Value Constructor. */
    Claim(Claim::Stretch horizontal, Claim::Stretch vertical)
        : m_horizontal(std::move(horizontal)), m_vertical(std::move(vertical)), m_ratios() {}

    /** Copy Constructor. */
    Claim(const Claim& other) = default;

    /** Returns the horizontal part of this Claim. */
    Stretch& get_horizontal() { return m_horizontal; }
    const Stretch& get_horizontal() const { return m_horizontal; }

    /** Returns the vertical part of this Claim. */
    Stretch& get_vertical() { return m_vertical; }
    const Stretch& get_vertical() const { return m_vertical; }

    /** Tests if both Stretches of this Claim are fixed. */
    bool is_fixed() const { return m_horizontal.is_fixed() && m_vertical.is_fixed(); }

    /** Sets a new minimum size of both Stretches, accomodates both the preferred and max size if necessary.
     * @param size  Minimum size, must be 0 <= size < INFINITY.
     */
    void set_min(const float width, const float height)
    {
        m_horizontal.set_min(width);
        m_vertical.set_min(height);
    }
    void set_min(const Size2f& size) { set_min(size.width, size.height); }

    /** Sets a new preferred size of both Stretches, accomodates both the min and max size if necessary.
     * @param size  Preferred size, must be 0 <= size < INFINITY.
     */
    void set_preferred(const float width, const float height)
    {
        m_horizontal.set_preferred(width);
        m_vertical.set_preferred(height);
    }
    void set_preferred(const Size2f& size) { set_preferred(size.width, size.height); }

    /** Sets a new maximum size of both Stretches, accomodates both the min and preferred size if necessary.
     * @param size  Maximum size, must be 0 <= size <= INFINITY.
     */
    void set_max(const float width, const float height)
    {
        m_horizontal.set_max(width);
        m_vertical.set_max(height);
    }
    void set_max(const Size2f& size) { set_max(size.width, size.height); }

    /** Sets the the scale factor of both Stretches.
     * @param factor    Scale factor, is clamped to 0 < factor < INFINITY.
     */
    void set_scale_factor(const float factor)
    {
        m_horizontal.set_scale_factor(factor);
        m_vertical.set_scale_factor(factor);
    }

    /** Sets the the priority of both Stretches (0 = default). */
    void set_priority(const int priority)
    {
        m_horizontal.set_priority(priority);
        m_vertical.set_priority(priority);
    }

    /** Sets both Stretches to a fixed size. */
    void set_fixed(const float width, const float height)
    {
        m_horizontal.set_fixed(width);
        m_vertical.set_fixed(height);
    }
    void set_fixed(const Size2f& size) { set_fixed(size.width, size.height); }

    /** Adds an offset to the min, max and preferred value of both Stretches.
     * Useful, for example, if you want to add a fixed "spacing" to the Claim of a Layout.
     * The offset can be negative.
     * Fields are truncated to be >= 0, invalid values are ignored.
     */
    void grow_by(const float offset)
    {
        m_horizontal.grow_by(offset);
        m_vertical.grow_by(offset);
    }

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
        return {m_ratios.first.get_height_for_width(), m_ratios.second.get_height_for_width()};
    }

    /** Sets the ratio constraint.
     * @param ratio_min    Width to Height (min/fixed value), is used as minimum value if the second parameter is set.
     * @param ratio_max    Width to Height (max value), 'ratio_min' is use by default.
     */
    void set_width_to_height(const float ratio_min, const float ratio_max = NAN);

    /** In-place max operator. */
    Claim& maxed(const Claim& other)
    {
        m_horizontal.maxed(other.m_horizontal);
        m_vertical.maxed(other.m_vertical);
        const std::pair<float, float> my_ratios    = get_width_to_height();
        const std::pair<float, float> other_ratios = other.get_width_to_height();
        set_width_to_height(min(my_ratios.first, other_ratios.first), max(my_ratios.second, other_ratios.second));
        return *this;
    }

    /** Assignment operator. */
    Claim& operator=(const Claim& other)
    {
        m_horizontal = other.m_horizontal;
        m_vertical   = other.m_vertical;
        m_ratios     = other.m_ratios;
        return *this;
    }

    /** Equality comparison operator. */
    bool operator==(const Claim& other) const
    {
        return (m_horizontal == other.m_horizontal
                && m_vertical == other.m_vertical
                && m_ratios == other.m_ratios);
    }

    /** Inequality comparison operator. */
    bool operator!=(const Claim& other) const { return !(*this == other); }

    /** Applies the constraints of this Claim to a given size.
     * @return  Constrainted size.
     */
    Size2f apply(const Size2f& size) const;

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
