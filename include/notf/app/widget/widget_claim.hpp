#pragma once

#include "notf/meta/real.hpp"

#include "notf/common/rational.hpp"
#include "notf/common/size2.hpp"

NOTF_OPEN_NAMESPACE

// claim ============================================================================================================ //

/// Every Widget has a Claim that determines how much space is alloted for it in its parent's Layout.
class WidgetClaim {

    // types ----------------------------------------------------------------------------------- //
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
            , m_max(is_nan(max) ? m_preferred : notf::max(max, m_preferred)) {}

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
        void set_min(const float min) {
            m_min = is_nan(min) ? 0 : max(min, 0);
            m_preferred = max(m_preferred, m_min);
            m_max = max(m_max, m_min);
        }

        /// Sets a new preferred size, accomodates the min or max size if necessary.
        /// @param preferred    Preferred size, must be >= 0.
        void set_preferred(const float preferred) {
            m_preferred = is_nan(preferred) ? 0 : max(preferred, 0);
            m_min = min(m_min, m_preferred);
            m_max = max(m_max, m_preferred);
        }

        /// Sets a new maximum size, accomodates both the min and preferred size if necessary.
        /// @param max  Maximum size, must be 0 <= size <= INFINITY.
        void set_max(const float max) {
            m_max = is_nan(max) ? 0 : notf::max(max, 0);
            m_preferred = min(m_preferred, m_max);
            m_min = min(m_min, m_max);
        }

        /// Sets a new scale factor.
        /// @param factor    Scale factor, is >= 0 and != infinity.
        void set_scale_factor(const float factor) {
            static constexpr float min_scale_factor = 0.00001f;
            if (factor <= 0 || !is_real(factor)) {
                m_scale_factor = min_scale_factor;
            } else {
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
        void grow_by(float offset) {
            offset = is_real(offset) ? max(offset, 0) : 0;
            m_preferred += offset;
            m_min += offset;
            m_max += offset;
        }

        /// Adds a negative offset to the min, max and preferred value.
        /// Useful, for example, if you want to undo the effect of growing a Claim.
        /// All values are clamped to be >= 0.
        /// @param offset   Offset, is truncated to be >= 0, invalid values are ignored.
        void shrink_by(float offset) {
            offset = is_real(offset) ? max(offset, 0) : 0;
            m_preferred = max(m_preferred - offset, 0);
            m_min = max(m_min - offset, 0);
            m_max = max(m_max - offset, 0);
        }

        /// In-place max operator.
        /// @param other    Stretch to max against.
        Stretch& maxed(const Stretch& other) {
            m_preferred = max(m_preferred, other.m_preferred);
            m_min = max(m_min, other.m_min);
            m_max = max(m_max, other.m_max);
            m_scale_factor = max(m_scale_factor, other.m_scale_factor);
            m_priority = max(m_priority, other.m_priority);
            return *this;
        }

        /// Equality comparison operator.
        /// @param other    Stretch to compare against.
        bool operator==(const Stretch& other) const {
            return (m_priority == other.m_priority && is_approx(m_preferred, other.m_preferred)
                    && is_approx(m_min, other.m_min) && is_approx(m_max, other.m_max)
                    && is_approx(m_scale_factor, other.m_scale_factor));
        }

        /// Inequality comparison operator.
        /// @param other    Stretch to compare against.
        bool operator!=(const Stretch& other) const { return !(*this == other); }

        /// In-place addition operator.
        /// @param other    Stretch to add.
        Stretch& operator+=(const Stretch& other) {
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

    // ========================================================================

    /// A Claim has two different ratio-constraints, one for the minimum ratio - one for the max.
    /// Each ratio is represented by a rational number (width / height).
    struct Ratios {
        friend class WidgetClaim;

        // methods --------------------------------------------------------- //
    private:
        /// Default constructor.
        Ratios() = default;

        /// Value constructor.
        /// @param lower_bound  Minimum ratio.
        /// @param upper_bound  Maximum ratio.
        Ratios(Ratioi lower_bound, Ratioi upper_bound) : m_min(std::move(lower_bound)), m_max(std::move(upper_bound)) {}

    public:
        /// Lower width / height limit.
        constexpr const Ratioi& get_lower_bound() const noexcept { return m_min; }

        /// Upper width / height limit.
        constexpr const Ratioi& get_upper_bound() const noexcept { return m_max; }

        /// Equality operator
        /// @param other    Ratio to compare against.
        constexpr bool operator==(const Ratios& other) const noexcept {
            return (m_min == other.m_min) && (m_max == other.m_max);
        }

        /// Ineequality operator
        /// @param other    Ratio to compare against.
        constexpr bool operator!=(const Ratios& other) const noexcept { return !(*this == other); }

        /// Combines these Ratio constraints with another one horizontally.
        /// @param other    Ratios to add on the horizontal axis.
        constexpr void combine_horizontal(const Ratios& other) noexcept {
            m_min = Ratioi(m_min.num() + other.m_min.num(), max(m_min.den(), other.m_min.den()));
            m_max = Ratioi(m_max.num() + other.m_max.num(), max(m_max.den(), other.m_max.den()));
        }

        /// Combines these Ratio constraints with another one vertically.
        /// @param other    Ratios to add on the vertical axis.
        constexpr void combine_vertical(const Ratios& other) noexcept {
            m_min = Ratioi(max(m_min.num(), other.m_min.num()), m_min.den() + other.m_min.den());
            m_max = Ratioi(max(m_max.num(), other.m_max.num()), m_max.den() + other.m_max.den());
        }

        // fields ---------------------------------------------------------- //
    private:
        /// Minimum ratio.
        Ratioi m_min;

        /// Maximum ratio.
        Ratioi m_max;
    };

    // methods --------------------------------------------------------------------------------- //
public:
    /// Default Constructor.
    WidgetClaim() = default;

    /// Value Constructor.
    /// @param horizontal   Horizontal Stretch.
    /// @param vertical     Vertical Stretch.
    WidgetClaim(WidgetClaim::Stretch horizontal, WidgetClaim::Stretch vertical)
        : m_horizontal(std::move(horizontal)), m_vertical(std::move(vertical)) {}

    ///@{
    /// Returns a Claim with fixed height and width.
    /// @param width    Width, is clamped to be >= 0.
    /// @param height   Height, is clamped to be >= 0.
    static WidgetClaim fixed(const float width, const float height) {
        WidgetClaim::Stretch horizontal, vertical;
        horizontal.set_fixed(width);
        vertical.set_fixed(height);
        return {horizontal, vertical};
    }
    static WidgetClaim fixed(const Size2f& size) { return fixed(size.width(), size.height()); }
    ///@}

    /// Returns a Claim with all limits set to zero.
    static WidgetClaim zero() {
        WidgetClaim::Stretch horizontal, vertical;
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
    void set_min(const float width, const float height) {
        m_horizontal.set_min(width);
        m_vertical.set_min(height);
    }
    void set_min(const Size2f& size) { set_min(size.width(), size.height()); }
    ///@}

    ///@{
    /// Sets a new preferred size of both Stretches, accomodates both the min and max size if necessary.
    /// @param size  Preferred size, must be 0 <= size < INFINITY in each dimension.
    void set_preferred(const float width, const float height) {
        m_horizontal.set_preferred(width);
        m_vertical.set_preferred(height);
    }
    void set_preferred(const Size2f& size) { set_preferred(size.width(), size.height()); }
    ///@}

    ///@{
    /// Sets a new maximum size of both Stretches, accomodates both the min and preferred size if necessary.
    /// @param size  Maximum size, must be 0 <= size <= INFINITY in each dimension.
    void set_max(const float width, const float height) {
        m_horizontal.set_max(width);
        m_vertical.set_max(height);
    }
    void set_max(const Size2f& size) { set_max(size.width(), size.height()); }
    ///@}

    /// Sets the the scale factor of both Stretches.
    /// @param factor    Scale factor, is clamped to 0 < factor < INFINITY.
    void set_scale_factor(const float factor) {
        m_horizontal.set_scale_factor(factor);
        m_vertical.set_scale_factor(factor);
    }

    /// Sets the the priority of both Stretches.
    /// @param priority    Priority.
    void set_priority(const int priority) {
        m_horizontal.set_priority(priority);
        m_vertical.set_priority(priority);
    }

    ///@{
    /// Sets both Stretches to a fixed size.
    /// @param width    Width, is clamped to be >= 0.
    /// @param height   Height, is clamped to be >= 0.
    void set_fixed(const float width, const float height) {
        m_horizontal.set_fixed(width);
        m_vertical.set_fixed(height);
    }
    void set_fixed(const Size2f& size) { set_fixed(size.width(), size.height()); }
    ///@}

    /// Adds a positive offset to the min, max and preferred value.
    /// Useful, for example, if you want to add a fixed "spacing" to the WidgetClaim of a Layout.
    /// @param offset   Offset, is truncated to be >= 0, invalid values are ignored.
    void grow_by(const float offset) {
        m_horizontal.grow_by(offset);
        m_vertical.grow_by(offset);
    }

    /// Adds a negative offset to the min, max and preferred value.
    /// Useful, for example, if you want to undo the effect of growing a WidgetClaim.
    /// All values are clamped to be >= 0.
    /// @param offset   Offset, is truncated to be >= 0, invalid values are ignored.
    void shrink_by(const float offset) {
        m_horizontal.shrink_by(offset);
        m_vertical.shrink_by(offset);
    }

    /// In-place, horizontal addition operator for WidgetClaims.
    /// @param other    WidgetClaim to add.
    WidgetClaim& add_horizontal(const WidgetClaim& other) {
        m_horizontal += other.m_horizontal;
        m_vertical.maxed(other.m_vertical);
        m_ratios.combine_horizontal(other.m_ratios);
        return *this;
    }

    /// In-place, vertical addition operator for Claims.
    /// @param other    Claim to add.
    WidgetClaim& add_vertical(const WidgetClaim& other) {
        m_horizontal.maxed(other.m_horizontal);
        m_vertical += other.m_vertical;
        m_ratios.combine_vertical(other.m_ratios);
        return *this;
    }

    /// Returns the min and max ratio constraints.
    /// (0, 0) means there exists no constraint.
    const Ratios& get_ratio_limits() const { return m_ratios; }

    /// Sets the ratio constraints (width / height)
    /// @param ratio_min    Min/fixed value, is used as minimum value if the second parameter is set.
    /// @param ratio_max    Max value, 'ratio_min' is use by default.
    void set_ratio_limits(Ratioi ratio_min, Ratioi ratio_max = Ratioi::zero());

    /// In-place max operator.
    /// @param other    Claim to max width.
    WidgetClaim& maxed(const WidgetClaim& other) {
        m_horizontal.maxed(other.m_horizontal);
        m_vertical.maxed(other.m_vertical);
        const Ratios& other_ratios = other.get_ratio_limits();
        set_ratio_limits(min(m_ratios.get_lower_bound(), other_ratios.get_lower_bound()),
                         max(m_ratios.get_upper_bound(), other_ratios.get_upper_bound()));
        return *this;
    }

    /// Equality comparison operator.
    /// @param other    Claim to compare width.
    bool operator==(const WidgetClaim& other) const {
        return (m_horizontal == other.m_horizontal && m_vertical == other.m_vertical && m_ratios == other.m_ratios);
    }

    /// Inequality comparison operator.
    /// @param other    Claim to compare width.
    bool operator!=(const WidgetClaim& other) const { return !(*this == other); }

    /// Applies the constraints of this Claim to a given size.
    /// @param size Input size.
    /// @return  Constrainted size.
    Size2f apply(Size2f size) const;

    // fields ---------------------------------------------------------------------------------- //
private:
    /// The vertical part of this Claim.
    Stretch m_horizontal;

    /// The horizontal part of this Claim.
    Stretch m_vertical;

    /// Minimum and maximum ratio scaling constraint.
    Ratios m_ratios;
};

// formatting ======================================================================================================= //

/// Prints the contents of a Claim::Stretch into a std::ostream.
/// @param out       Output stream, implicitly passed with the << operator.
/// @param stretch   Claim::Stretch to print.
/// @return          Output stream for further output.
std::ostream& operator<<(std::ostream& out, const WidgetClaim::Stretch& stretch);

/// Prints the contents of a Claim into a std::ostream.
/// @param out   Output stream, implicitly passed with the << operator.
/// @param claim Claim to print.
/// @return      Output stream for further output.
std::ostream& operator<<(std::ostream& out, const WidgetClaim& aabr);

/// Prints the contents of a Stretch into a std::ostream.
/// @param os       Output stream, implicitly passed with the << operator.
/// @param stretch  Stretch to print.
/// @return         Output stream for further output.
inline std::ostream& operator<<(std::ostream& out, const notf::WidgetClaim::Stretch& stretch) {
    return out << fmt::format("{}", stretch);
}

/// Prints the contents of a Claim into a std::ostream.
/// @param os       Output stream, implicitly passed with the << operator.
/// @param claim    Claim to print.
/// @return         Output stream for further output.
inline std::ostream& operator<<(std::ostream& out, const notf::WidgetClaim& claim) {
    return out << fmt::format("{}", claim);
}

NOTF_CLOSE_NAMESPACE

namespace fmt {

template<>
struct formatter<notf::WidgetClaim::Stretch> {
    template<typename ParseContext>
    constexpr auto parse(ParseContext& ctx) {
        return ctx.begin();
    }

    template<typename FormatContext>
    auto format(const notf::WidgetClaim::Stretch& stretch, FormatContext& ctx) {
        return format_to(ctx.begin(), "WidgetClaim::Stretch({} <= {} <= {}, factor: {}, priority: {})", //
                         stretch.get_min(), stretch.get_preferred(), stretch.get_max(),                 //
                         stretch.get_scale_factor(), stretch.get_priority());
    }
};

template<>
struct formatter<notf::WidgetClaim> {
    template<typename ParseContext>
    constexpr auto parse(ParseContext& ctx) {
        return ctx.begin();
    }

    template<typename FormatContext>
    auto format(const notf::WidgetClaim& claim, FormatContext& ctx) {
        return format_to(ctx.begin(),
                         "WidgetClaim(horizontal: [{}]\n"              //
                         "      vertical:   [{}]\n"                    //
                         "      ratio min:   {}\n"                     //
                         "      ratio max:   {})",                     //
                         claim.get_horizontal(), claim.get_vertical(), //
                         claim.get_ratio_limits().get_lower_bound(), claim.get_ratio_limits().get_upper_bound());
    }
};

} // namespace fmt

// std::hash ======================================================================================================== //

namespace std {

/// std::hash specialization for notf::Claim::Stretch.
template<>
struct hash<notf::WidgetClaim::Stretch> {
    size_t operator()(const notf::WidgetClaim::Stretch& stretch) const {
        return notf::hash(stretch.get_preferred(), stretch.get_min(), stretch.get_max(), stretch.get_scale_factor(),
                          stretch.get_priority());
    }
};

/// std::hash specialization for notf::Claim.
template<>
struct hash<notf::WidgetClaim> {
    size_t operator()(const notf::WidgetClaim& claim) const {
        const notf::WidgetClaim::Ratios& ratio_limits = claim.get_ratio_limits();
        return notf::hash(claim.get_horizontal(), claim.get_horizontal(), ratio_limits.get_lower_bound(),
                          ratio_limits.get_upper_bound());
    }
};

} // namespace std
