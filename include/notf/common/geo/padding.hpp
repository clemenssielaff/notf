#pragma once

#include "notf/meta/hash.hpp"

#include "notf/common/geo/size2.hpp"

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
    constexpr Padding() noexcept = default;

    /// Even padding on all sides.
    static constexpr Padding all(const element_t padding) noexcept { return {padding, padding, padding, padding}; }

    /// No padding.
    static constexpr Padding none() noexcept { return {0, 0, 0, 0}; }

    /// Horizontal padding, sets both `left` and `right`.
    static constexpr Padding horizontal(const element_t padding) noexcept { return {0, padding, 0, padding}; }

    /// Vertical padding, sets both `top` and `bottom`.
    static constexpr Padding vertical(const element_t padding) noexcept { return {padding, 0, padding, 0}; }

    /// Checks if any of the sides is padding.
    constexpr bool is_padding() const noexcept { return !(top == 0 && right == 0 && bottom == 0 && left == 0); }

    /// Checks if this Padding is valid (all sides have values >= 0).
    constexpr bool is_valid() const noexcept { return top >= 0 && right >= 0 && bottom >= 0 && left >= 0; }

    /// Sum of the two horizontal padding sizes.
    constexpr element_t get_width() const noexcept { return left + right; }

    /// Sum of the two vertical padding sizes.
    constexpr element_t get_height() const noexcept { return top + bottom; }

    /// Size of the padding, combining its width and height.
    constexpr Size2<element_t> get_size() const noexcept { return {get_width(), get_height()}; }

    /// Equality operator.
    constexpr bool operator==(const Padding& other) const noexcept {
        return (is_approx(other.top, top) && is_approx(other.right, right) && is_approx(other.bottom, bottom)
                && is_approx(other.left, left));
    }

    /// Inequality operator.
    constexpr bool operator!=(const Padding& other) const noexcept {
        return (!is_approx(other.top, top) || !is_approx(other.right, right) || !is_approx(other.bottom, bottom)
                || !is_approx(other.left, left));
    }

    // fields ---------------------------------------------------------------------------------- //
public:
    /// @{
    /// Padding sides in the same order as css margins.
    element_t top;
    element_t right;
    element_t bottom;
    element_t left;
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
