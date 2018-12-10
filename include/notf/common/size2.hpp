#pragma once

#include "notf/common/arithmetic.hpp"

NOTF_OPEN_NAMESPACE

// size2 ============================================================================================================ //

namespace detail {

/// Two-dimensional size.
template<class Element>
struct Size2 : public Arithmetic<Size2<Element>, Element, Element, 2> {

    // types ----------------------------------------------------------------------------------- //
public:
    /// Base class.
    using super_t = Arithmetic<Size2<Element>, Element, Element, 2>;

    /// Scalar type used by this size type.
    using element_t = Element;

    // methods --------------------------------------------------------------------------------- //
public:
    /// Default (non-initializing) constructor..
    Size2() = default;

    /// Forwarding constructor.
    template<class... Args>
    constexpr Size2(Args&&... args) : super_t(std::forward<Args>(args)...) {}

    /// Creates and returns an invalid Size2 instance.
    constexpr static Size2 invalid() { return {-1, -1}; }

    /// Creates and returns zero Size2 instance.
    constexpr static Size2 zero() { return {0, 0}; }

    /// The "most wrong" Size2 (maximal negative area).
    /// Is useful as the starting point for defining the union of multiple Size2.
    constexpr static Size2 wrongest() { return {min_value<element_t>(), min_value<element_t>()}; }

    /// Name of this Size2 type.
    static constexpr const char* get_name() {
        if constexpr (std::is_same_v<Element, float>) {
            return "Size2f";
        } else if constexpr (std::is_same_v<Element, double>) {
            return "Size2d";
        } else if constexpr (std::is_same_v<Element, int>) {
            return "Size2i";
        } else if constexpr (std::is_same_v<Element, short>) {
            return "Size2s";
        }
    }

    /// Width
    constexpr element_t& width() noexcept { return data[0]; }
    constexpr const element_t& width() const noexcept { return data[0]; }

    /// Height
    constexpr element_t& height() noexcept { return data[1]; }
    constexpr const element_t& height() const noexcept { return data[1]; }

    /// Tests if this Size is valid (>=0) in both dimensions.
    constexpr bool is_valid() const noexcept { return width() >= 0 && height() >= 0; }

    /// Checks if the size has the same height and width.
    constexpr bool is_square() const noexcept { return (abs(width()) - abs(height())) <= precision_high<element_t>(); }

    /// Returns the area of a rectangle of this Size.
    /// Always returns 0, if the size is invalid.
    constexpr element_t get_area() const noexcept { return notf::max(0, width() * height()); }

    // fields ---------------------------------------------------------------------------------- //
public:
    /// Value data array.
    using super_t::data;
};

} // namespace detail

using Size2f = detail::Size2<float>;
using Size2d = detail::Size2<double>;
using Size2i = detail::Size2<int>;
using Size2s = detail::Size2<short>;

NOTF_CLOSE_NAMESPACE

// std::hash ======================================================================================================== //

namespace std {

/// std::hash specialization for notf::Size2.
template<class Element>
struct hash<notf::detail::Size2<Element>> {
    size_t operator()(const notf::detail::Size2<Element>& size2) const {
        return notf::hash(notf::to_number(notf::detail::HashID::SIZE2), size2.get_hash());
    }
};

} // namespace std

// formatting ======================================================================================================= //

namespace fmt {

template<class Element>
struct formatter<notf::detail::Size2<Element>> {
    using type = notf::detail::Size2<Element>;

    template<typename ParseContext>
    constexpr auto parse(ParseContext& ctx) {
        return ctx.begin();
    }

    template<typename FormatContext>
    auto format(const type& size, FormatContext& ctx) {
        return format_to(ctx.begin(), "{}({}x{})", type::get_name(), size.width, size.height);
    }
};

} // namespace fmt

/// Prints the contents of a vector into a std::ostream.
/// @param os   Output stream, implicitly passed with the << operator.
/// @param vec  Vector to print.
/// @return Output stream for further output.
template<class Element>
std::ostream& operator<<(std::ostream& out, const notf::detail::Size2<Element>& vec) {
    return out << fmt::format("{}", vec);
}

// compile time tests =============================================================================================== //

static_assert(std::is_pod_v<notf::Size2f>);
static_assert(std::is_pod_v<notf::Size2d>);
static_assert(std::is_pod_v<notf::Size2i>);
static_assert(std::is_pod_v<notf::Size2s>);

static_assert(sizeof(notf::Size2f) == sizeof(float) * 2);
static_assert(sizeof(notf::Size2d) == sizeof(double) * 2);
static_assert(sizeof(notf::Size2i) == sizeof(int) * 2);
static_assert(sizeof(notf::Size2s) == sizeof(short) * 2);
