#pragma once

#include "common/size2.hpp"

NOTF_OPEN_NAMESPACE

// ================================================================================================================== //

/// Every Widget has a Claim that determines how much space is alloted for it in its parent's Layout.
/// The user can declare Claims manually, although Layouts usually have a mechanism to calculate their own Claim based
/// on the combined Claims of their children. A Claim is made up of serveral parts:
///
/// Stretches
/// ---------
/// A Claim has 2 `Stretch` fields, one for its horizontal and one for its vertical expansion.
/// Each Stretch consists of a `minimum` value, a `maximum` and a preferred value.
/// Usually, the Widget assumes its `preferred` size first and is then regulated up or down, depending on how much space
/// is left in its parent's Layout.
///
/// The `Stretch factor` of a Strech determines, how fast a Widget grows in relation to its siblings. Two Widgets with a
/// Stretch factors of 1 each, will grow at the same rate when more space becomes available. If one of them had a
/// Stretch factor of 2, it would grow twice as fast as the other, until it reaches its maximum. Conversely, a Stretch
/// factor of 0.5 would make it grow only half as fast. Stretch factors have to be larger than zero, assigning a Stretch
/// factor of <= 0 will cause a warning and the factor will be clamped to a value > 0.
///
/// The `priority` of a Stretch comes into play, when you want one Widget to fully expand before any others are even
/// considered.
/// If you have 3 Widgetss A, B and C and C has a priority of 1, while A and B have a priority of 0, then C will take
/// up all available space without giving any to A and B. Only after C has reached its maximum size is the additional
/// space distributed to A and B (using their individual Stretch factors). Conversely, if the available space should
/// shrink then A and B are the first ones to give up their additional space. C will only shrink after A and B have both
/// reached their minimum size.
///
/// You can modify the two Stretches of a Claim individually, or set them both using the Claim's functions.
class Claim {

    // types -------------------------------------------------------------------------------------------------------- //
public:
    /// Horizontal or vertical Stretch of the Claim.
    class Stretch {

        // methods ------------------------------------------------------------
    public:
        /// Default Constructor.
        Stretch() = default;

        /// Value Constructor.
        /// After construction the following contraints hold:
        ///
        ///     0 <= min <= preferred <= max <= infinity
        ///
        /// @param preferred    Preferred size in local units.
        /// @param min          Minimum size, defaults to `preferred`.
        /// @param max          Maximum size, defaults to `preferred`.
        Stretch(const float preferred, const float min = NAN, const float max = NAN)
            : m_preferred(is_nan(preferred) ? 0 : notf::max(preferred, 0))
            , m_min(is_real(min) ? clamp(min, 0, m_preferred) : m_preferred)
            , m_max(is_nan(max) ? m_preferred : notf::max(max, m_preferred))
        {}

        /// Minimum size in local units, is 0 <= min <= preferred.
        float get_min() const { return m_min; }

        /// Preferred size in local units, is >= 0.
        float get_preferred() const { return m_preferred; }

        /// Maximum size in local units, is >= preferred.
        float get_max() const { return m_max; }

        /// Returns the scale factor.
        float get_scale_factor() const { return m_scale_factor; }

        /// Returns the scale priority.
        int get_priority() const { return m_priority; }

        /// Tests if this Stretch is a fixed size where all 3 values are the same.
        bool is_fixed() const { return is_approx(m_min, m_preferred) && is_approx(m_preferred, m_max); }

        /// Sets a new minimum size, accomodates both the preferred and max size if necessary.
        /// @param min  Minimum size, must be 0 <= size < INFINITY.
        void set_min(const float min)
        {
            m_min = is_nan(min) ? 0 : max(min, 0);
            m_preferred = max(m_preferred, m_min);
            m_max = max(m_max, m_min);
        }

        /// Sets a new preferred size, accomodates the min or max size if necessary.
        /// @param preferred    Preferred size, must be >= 0.
        void set_preferred(const float preferred)
        {
            m_preferred = is_nan(preferred) ? 0 : max(preferred, 0);
            m_min = min(m_min, m_preferred);
            m_max = max(m_max, m_preferred);
        }

        /// Sets a new maximum size, accomodates both the min and preferred size if necessary.
        /// @param max  Maximum size, must be 0 <= size <= INFINITY.
        void set_max(const float max)
        {
            m_max = is_nan(max) ? 0 : notf::max(max, 0);
            m_preferred = min(m_preferred, m_max);
            m_min = min(m_min, m_max);
        }

        /// Sets a new scale factor.
        /// @param factor    Scale factor, is >= 0 and != infinity.
        void set_scale_factor(const float factor)
        {
            static constexpr float min_scale_factor = 0.00001f;
            if (factor <= 0 || !is_real(factor)) {
                m_scale_factor = min_scale_factor;
            }
            else {
                m_scale_factor = factor;
            }
        }

        /// Sets a new scaling priority (0 = default).
        /// @param priority     New scaling priority.
        void set_priority(const int priority) { m_priority = priority; }

        /// Sets a fixed size.
        /// @param size     New min/max and preferred size.
        void set_fixed(const float size) { m_min = m_max = m_preferred = max(size, 0); }

        /// Adds a positive offset to the min, max and preferred value.
        /// Useful, for example, if you want to add a fixed "spacing" to the Claim of a Layout.
        /// @param offset   Offset, is truncated to be >= 0, invalid values are ignored.
        void grow_by(float offset)
        {
            offset = is_real(offset) ? max(offset, 0) : 0;
            m_preferred += offset;
            m_min += offset;
            m_max += offset;
        }

        /// Adds a negative offset to the min, max and preferred value.
        /// Useful, for example, if you want to undo the effect of growing a Claim.
        /// All values are clamped to be >= 0.
        /// @param offset   Offset, is truncated to be >= 0, invalid values are ignored.
        void shrink_by(float offset)
        {
            offset = is_real(offset) ? max(offset, 0) : 0;
            m_preferred = max(m_preferred - offset, 0);
            m_min = max(m_min - offset, 0);
            m_max = max(m_max - offset, 0);
        }

        /// In-place max operator.
        /// @param other    Stretch to max against.
        Stretch& maxed(const Stretch& other)
        {
            m_preferred = max(m_preferred, other.m_preferred);
            m_min = max(m_min, other.m_min);
            m_max = max(m_max, other.m_max);
            m_scale_factor = max(m_scale_factor, other.m_scale_factor);
            m_priority = max(m_priority, other.m_priority);
            return *this;
        }

        /// Equality comparison operator.
        /// @param other    Stretch to compare against.
        bool operator==(const Stretch& other) const
        {
            return (m_priority == other.m_priority && is_approx(m_preferred, other.m_preferred)
                    && is_approx(m_min, other.m_min) && is_approx(m_max, other.m_max)
                    && is_approx(m_scale_factor, other.m_scale_factor));
        }

        /// Inequality comparison operator.
        /// @param other    Stretch to compare against.
        bool operator!=(const Stretch& other) const { return !(*this == other); }

        /// In-place addition operator.
        /// @param other    Stretch to add.
        Stretch& operator+=(const Stretch& other)
        {
            m_preferred += other.m_preferred;
            m_min += other.m_min;
            m_max += other.m_max;
            m_scale_factor = max(m_scale_factor, other.m_scale_factor);
            m_priority = max(m_priority, other.m_priority);
            return *this;
        }

        // fields -------------------------------------------------------------
    private:
        /// Preferred size, is: min <= size <= max.
        float m_preferred = 0;

        /// Minimal size, is: 0 <= size <= preferred.
        float m_min = 0;

        /// Maximal size, is: preferred <= size <= INFINITY.
        float m_max = INFINITY;

        /// Scale factor, 0 means no scaling, is: 0 <= factor < INFINITY
        float m_scale_factor = 1;

        /// @brief Scaling priority, is INT_MIN <= priority <= INT_MAX.
        int m_priority = 0;
    };

    // types -------------------------------------------------------------------------------------------------------- //
private:
    /// A height-for-width ratio constraint of the Claim.
    /// Is its own class so two Ratios can be properly added.
    /// A value of zero means no Ratio constraint.
    class Ratio {

        // methods ------------------------------------------------------------
    public:
        /// Default Constructor.
        Ratio() = default;

        /// Value Constructor.
        /// Setting one or both values to zero, results in an invalid Ratio.
        /// @param width    Width in units, is 0 < width < INFINITY
        /// @param height   Height in units, is 0 < height < INFINITY
        Ratio(const float width, const float height = 1) : m_width(width), m_height(height)
        {
            if (!is_real(width) || !is_real(height) || width <= 0 || height <= 0) {
                m_width = 0;
                m_height = 0;
            }
        }

        /// Tests if this Ratio is valid.
        bool is_valid() const { return !is_zero(m_width) && !is_zero(m_height); }

        /// Returns the ratio, is 0 if invalid.
        float height_for_width() const
        {
            if (!is_valid()) {
                return 0;
            }
            return m_height / m_width;
        }

        /// In-place, horizontal addition operator.
        /// @param other    Ratio to add on the horizontal axis.
        Ratio& add_horizontal(const Ratio& other)
        {
            m_width += other.m_width;
            m_height = max(m_height, other.m_height);
            return *this;
        }

        /// In-place, vertical addition operator.
        /// @param other    Ratio to add on the vertical axis.
        Ratio& add_vertical(const Ratio& other)
        {
            m_width = max(m_width, other.m_width);
            m_height += other.m_height;
            return *this;
        }

        /// Equality operator
        /// @param other    Ratio to compare against.
        bool operator==(const Ratio& other) const
        {
            return is_approx(m_width, other.m_width) && is_approx(m_height, other.m_height);
        }

        /// Ineequality operator
        /// @param other    Ratio to compare against.
        bool operator!=(const Ratio& other) const { return !(*this == other); }

        // fields -------------------------------------------------------------
    private:
        /// Width.
        float m_width = 0;

        /// Height.
        float m_height = 0;
    };

    /// A Claim has two different ratio-constraints, one for the minimum ratio - one for the max.
    struct Ratios {
        /// Equality operator
        /// @param other    Ratio to compare against.
        bool operator==(const Ratios& other) const
        {
            return (lower_bound == other.lower_bound) && (upper_bound == other.upper_bound);
        }

        /// Ineequality operator
        /// @param other    Ratio to compare against.
        bool operator!=(const Ratios& other) const { return !(*this == other); }

        // fields -------------------------------------------------------------
    public:
        /// Minimum Ratio.
        Ratio lower_bound = {};

        /// Maximum Ratio.
        Ratio upper_bound = {};
    };

    // methods ------------------------------------------------------------------------------------------------------ //
public:
    /// Default Constructor.
    Claim() = default;

    /// Value Constructor.
    /// @param horizontal   Horizontal Stretch.
    /// @param vertical     Vertical Stretch.
    Claim(Claim::Stretch horizontal, Claim::Stretch vertical)
        : m_horizontal(std::move(horizontal)), m_vertical(std::move(vertical))
    {}

    ///@{
    /// Returns a Claim with fixed height and width.
    /// @param width    Width, is clamped to be >= 0.
    /// @param height   Height, is clamped to be >= 0.
    static Claim fixed(const float width, const float height)
    {
        Claim::Stretch horizontal, vertical;
        horizontal.set_fixed(width);
        vertical.set_fixed(height);
        return {horizontal, vertical};
    }
    static Claim fixed(const Size2f& size) { return fixed(size.width, size.height); }
    ///@}

    /// Returns a Claim with all limits set to zero.
    static Claim zero()
    {
        Claim::Stretch horizontal, vertical;
        horizontal.set_fixed(0);
        vertical.set_fixed(0);
        return {horizontal, vertical};
    }

    ///@{
    /// Returns the horizontal part of this Claim.
    Stretch& get_horizontal() { return m_horizontal; }
    const Stretch& get_horizontal() const { return m_horizontal; }
    ///@}

    ///@{
    /// Returns the vertical part of this Claim.
    Stretch& get_vertical() { return m_vertical; }
    const Stretch& get_vertical() const { return m_vertical; }
    ///@}

    /// Tests if both Stretches of this Claim are fixed.
    bool is_fixed() const { return m_horizontal.is_fixed() && m_vertical.is_fixed(); }

    ///@{
    /// Sets a new minimum size of both Stretches, accomodates both the preferred and max size if necessary.
    /// @param size  Minimum size, must be 0 <= size < INFINITY in each dimension.
    void set_min(const float width, const float height)
    {
        m_horizontal.set_min(width);
        m_vertical.set_min(height);
    }
    void set_min(const Size2f& size) { set_min(size.width, size.height); }
    ///@}

    ///@{
    /// Sets a new preferred size of both Stretches, accomodates both the min and max size if necessary.
    /// @param size  Preferred size, must be 0 <= size < INFINITY in each dimension.
    void set_preferred(const float width, const float height)
    {
        m_horizontal.set_preferred(width);
        m_vertical.set_preferred(height);
    }
    void set_preferred(const Size2f& size) { set_preferred(size.width, size.height); }
    ///@}

    ///@{
    /// Sets a new maximum size of both Stretches, accomodates both the min and preferred size if necessary.
    /// @param size  Maximum size, must be 0 <= size <= INFINITY in each dimension.
    void set_max(const float width, const float height)
    {
        m_horizontal.set_max(width);
        m_vertical.set_max(height);
    }
    void set_max(const Size2f& size) { set_max(size.width, size.height); }
    ///@}

    /// Sets the the scale factor of both Stretches.
    /// @param factor    Scale factor, is clamped to 0 < factor < INFINITY.
    void set_scale_factor(const float factor)
    {
        m_horizontal.set_scale_factor(factor);
        m_vertical.set_scale_factor(factor);
    }

    /// Sets the the priority of both Stretches.
    /// @param priority    Priority.
    void set_priority(const int priority)
    {
        m_horizontal.set_priority(priority);
        m_vertical.set_priority(priority);
    }

    ///@{
    /// Sets both Stretches to a fixed size.
    /// @param width    Width, is clamped to be >= 0.
    /// @param height   Height, is clamped to be >= 0.
    void set_fixed(const float width, const float height)
    {
        m_horizontal.set_fixed(width);
        m_vertical.set_fixed(height);
    }
    void set_fixed(const Size2f& size) { set_fixed(size.width, size.height); }
    ///@}

    /// Adds a positive offset to the min, max and preferred value.
    /// Useful, for example, if you want to add a fixed "spacing" to the Claim of a Layout.
    /// @param offset   Offset, is truncated to be >= 0, invalid values are ignored.
    void grow_by(const float offset)
    {
        m_horizontal.grow_by(offset);
        m_vertical.grow_by(offset);
    }

    /// Adds a negative offset to the min, max and preferred value.
    /// Useful, for example, if you want to undo the effect of growing a Claim.
    /// All values are clamped to be >= 0.
    /// @param offset   Offset, is truncated to be >= 0, invalid values are ignored.
    void shrink_by(const float offset)
    {
        m_horizontal.shrink_by(offset);
        m_vertical.shrink_by(offset);
    }

    /// In-place, horizontal addition operator for Claims.
    /// @param other    Claim to add.
    Claim& add_horizontal(const Claim& other)
    {
        m_horizontal += other.m_horizontal;
        m_vertical.maxed(other.m_vertical);
        m_ratios = {
            m_ratios.lower_bound.add_horizontal(other.m_ratios.lower_bound),
            m_ratios.upper_bound.add_horizontal(other.m_ratios.upper_bound),
        };
        return *this;
    }

    /// In-place, vertical addition operator for Claims.
    /// @param other    Claim to add.
    Claim& add_vertical(const Claim& other)
    {
        m_horizontal.maxed(other.m_horizontal);
        m_vertical += other.m_vertical;
        m_ratios = {
            m_ratios.lower_bound.add_vertical(other.m_ratios.lower_bound),
            m_ratios.upper_bound.add_vertical(other.m_ratios.upper_bound),
        };
        return *this;
    }

    /// Returns the min and max ratio constraints.
    /// (0, 0) means there exists no constraint.
    std::pair<float, float> get_width_to_height() const
    {
        return {m_ratios.lower_bound.height_for_width(), m_ratios.upper_bound.height_for_width()};
    }

    /// Sets the ratio constraint.
    /// @param ratio_min    Width to Height (min/fixed value), is used as minimum value if the second parameter is set.
    /// @param ratio_max    Width to Height (max value), 'ratio_min' is use by default.
    void set_width_to_height(float ratio_min, float ratio_max = NAN);

    /// In-place max operator.
    /// @param other    Claim to max width.
    Claim& maxed(const Claim& other)
    {
        m_horizontal.maxed(other.m_horizontal);
        m_vertical.maxed(other.m_vertical);
        const std::pair<float, float> my_ratios = get_width_to_height();
        const std::pair<float, float> other_ratios = other.get_width_to_height();
        set_width_to_height(min(my_ratios.first, other_ratios.first), max(my_ratios.second, other_ratios.second));
        return *this;
    }

    /// Equality comparison operator.
    /// @param other    Claim to compare width.
    bool operator==(const Claim& other) const
    {
        return (m_horizontal == other.m_horizontal && m_vertical == other.m_vertical && m_ratios == other.m_ratios);
    }

    /// Inequality comparison operator.
    /// @param other    Claim to compare width.
    bool operator!=(const Claim& other) const { return !(*this == other); }

    /// Applies the constraints of this Claim to a given size.
    /// @param size Input size.
    /// @return  Constrainted size.
    Size2f apply(const Size2f& size) const;

    // fields ------------------------------------------------------------------------------------------------------- //
private:
    /// The vertical part of this Claim.
    Stretch m_horizontal;

    /// The horizontal part of this Claim.
    Stretch m_vertical;

    /// Minimum and maximum ratio scaling constraint.
    Ratios m_ratios;
};

// ================================================================================================================== //

/// Prints the contents of a Claim::Stretch into a std::ostream.
/// @param out       Output stream, implicitly passed with the << operator.
/// @param stretch   Claim::Stretch to print.
/// @return          Output stream for further output.
std::ostream& operator<<(std::ostream& out, const Claim::Stretch& stretch);

/// Prints the contents of a Claim into a std::ostream.
/// @param out   Output stream, implicitly passed with the << operator.
/// @param claim Claim to print.
/// @return      Output stream for further output.
std::ostream& operator<<(std::ostream& out, const Claim& aabr);

NOTF_CLOSE_NAMESPACE

//== std::hash ====================================================================================================== //

namespace std {

/// std::hash specialization for notf::Claim::Stretch.
template<>
struct hash<notf::Claim::Stretch> {
    size_t operator()(const notf::Claim::Stretch& stretch) const
    {
        return notf::hash(stretch.get_preferred(), stretch.get_min(), stretch.get_max(), stretch.get_scale_factor(),
                          stretch.get_priority());
    }
};

/// std::hash specialization for notf::Claim.
template<>
struct hash<notf::Claim> {
    size_t operator()(const notf::Claim& claim) const
    {
        const std::pair<float, float> ratio = claim.get_width_to_height();
        return notf::hash(claim.get_horizontal(), claim.get_horizontal(), ratio.first, ratio.second);
    }
};

} // namespace std
