#pragma once

#include "notf/meta/hash.hpp"

#include "notf/app/fwd.hpp"

NOTF_OPEN_NAMESPACE

// padding ========================================================================================================== //

namespace detail {

/// 4-sided Padding to use in Layouts.
/// Uses the same order as css margins, that is starting at top and then clockwise (top / right / bottom/ left).
template<class Element>
struct Padding {

    // types ----------------------------------------------------------------------------------- //
public:
    /// Element type.
    using element_t = Element;

    // methods --------------------------------------------------------------------------------- //
public:
    /// Default constructor.
    Padding() = default;

    /// Even padding on all sides.
    static Padding all(const float padding) { return {padding, padding, padding, padding}; }

    /// No padding.
    static Padding none() { return {0.f, 0.f, 0.f, 0.f}; }

    /// Horizontal padding, sets both `left` and `right`.
    static Padding horizontal(const float padding) { return {0.f, padding, 0.f, padding}; }

    /// Vertical padding, sets both `top` and `bottom`.
    static Padding vertical(const float padding) { return {padding, 0.f, padding, 0.f}; }

    /// Checks if any of the sides is padding.
    bool is_padding() const { return !(top == 0.f && right == 0.f && bottom == 0.f && left == 0.f); }

    /// Checks if this Padding is valid (all sides have values >= 0).
    bool is_valid() const { return top >= 0.f && right >= 0.f && bottom >= 0.f && left >= 0.f; }

    /// Sum of the two horizontal padding sizes.
    float get_width() const { return left + right; }

    /// Sum of the two vertical padding sizes.
    float get_height() const { return top + bottom; }

    /// Equality operator.
    bool operator==(const Padding& other) const {
        return (is_approx(other.top, top) && is_approx(other.right, right) && is_approx(other.bottom, bottom)
                && is_approx(other.left, left));
    }

    /// Inequality operator.
    bool operator!=(const Padding& other) const {
        return (!is_approx(other.top, top) || !is_approx(other.right, right) || !is_approx(other.bottom, bottom)
                || !is_approx(other.left, left));
    }

    // fields ---------------------------------------------------------------------------------- //
public:
    /// @{
    /// Padding sides in the same order as css margins.
    float top;
    float right;
    float bottom;
    float left;
    /// @}
};

} // namespace detail

// formatting ======================================================================================================= //

/// @{
/// Prints the contents of a Padding object into a std::ostream.
/// @param out      Output stream, implicitly passed with the << operator.
/// @param padding  Padding to print.
/// @return Output stream for further output.
inline std::ostream& operator<<(std::ostream& out, const Paddingf& padding) {
    return out << fmt::format("{}", padding);
}
inline std::ostream& operator<<(std::ostream& out, const Paddingi& padding) {
    return out << fmt::format("{}", padding);
}
/// @}

NOTF_CLOSE_NAMESPACE

namespace fmt {

template<class Element>
struct formatter<notf::detail::Padding<Element>> {
    using type = notf::detail::Padding<Element>;

    template<typename ParseContext>
    constexpr auto parse(ParseContext& ctx) {
        return ctx.begin();
    }

    template<typename FormatContext>
    auto format(const type& padding, FormatContext& ctx) {
        return format_to(ctx.begin(), "Padding({}, {}, {}, {})", padding.top, padding.right, padding.bottom,
                         padding.left);
    }
    // TODO: mention padding element
};

} // namespace fmt

// std::hash ======================================================================================================== //

/// std::hash specialization for Circle.
template<class Element>
struct std::hash<notf::detail::Padding<Element>> {
    size_t operator()(const notf::detail::Padding<Element>& padding) const {
        return notf::hash(static_cast<size_t>(notf::detail::HashID::PADDING), padding.top, padding.right,
                          padding.bottom, padding.left);
    }
};
