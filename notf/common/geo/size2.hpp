#pragma once

#include "notf/common/geo/arithmetic.hpp"

NOTF_OPEN_NAMESPACE

// size2 ============================================================================================================ //

namespace detail {

/// Two-dimensional size.
template<class Element>
struct Size2 : public Arithmetic<Size2<Element>, Element, 2> {

    // types ----------------------------------------------------------------------------------- //
private:
    /// Base class.
    using super_t = Arithmetic<Size2<Element>, Element, 2>;

public:
    /// Scalar type used by this size type.
    using element_t = Element;

    // methods --------------------------------------------------------------------------------- //
public:
    /// Default (non-initializing) constructor.
    constexpr Size2() noexcept = default;

    /// Forwarding constructor.
    template<class... Args>
    constexpr Size2(Args&&... args) noexcept : super_t{std::forward<Args>(args)...} {}

    /// Name of this Size2 type.
    static constexpr const char* get_type_name() {
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

    /// Returns an invalid Size2 instance.
    constexpr static Size2 invalid() noexcept { return {-1, -1}; }

    /// The largest representable Size2.
    constexpr static Size2 largest() noexcept {
        Size2 result{};
        result[0] = lowest_v<element_t>;
        result[1] = highest_v<element_t>;
        return result;
    }

    /// The "most wrong" Size2 (maximal negative area).
    /// Is useful as the starting point for defining the union of multiple Size2.
    constexpr static Size2 wrongest() noexcept { return {lowest_v<element_t>, lowest_v<element_t>}; }

    /// Width
    constexpr const element_t& get_width() const noexcept { return data[0]; }

    /// Set width
    /// @param width    New width.
    constexpr const Size2& set_width(const element_t width) noexcept {
        _width() = width;
        return *this;
    }

    /// Height
    constexpr const element_t& get_height() const noexcept { return data[1]; }

    /// Set height
    /// @param width    New height.
    constexpr const Size2& set_height(const element_t height) noexcept {
        _height() = height;
        return *this;
    }

    /// Tests if this Size2 is valid (>=0) in both dimensions.
    constexpr bool is_valid() const noexcept { return get_width() >= 0 && get_height() >= 0; }

    /// Checks if the Size2 has the same height and width.
    constexpr bool is_square() const noexcept {
        return abs(get_width()) - abs(get_height()) <= precision_high<element_t>();
    }

    /// Returns the area of a rectangle of this Size2.
    /// Always returns 0, if the size is invalid.
    constexpr element_t get_area() const noexcept { return max(0, get_width() * get_height()); }

private:
    constexpr element_t& _width() noexcept { return data[0]; }
    constexpr element_t& _height() noexcept { return data[1]; }

    // fields ---------------------------------------------------------------------------------- //
public:
    /// Value data array.
    using super_t::data;
};

} // namespace detail

// formatting ======================================================================================================= //

/// Prints the contents of a vector into a std::ostream.
/// @param os   Output stream, implicitly passed with the << operator.
/// @param vec  Vector to print.
/// @return Output stream for further output.
template<class Element>
std::ostream& operator<<(std::ostream& out, const notf::detail::Size2<Element>& vec) {
    return out << fmt::format("{}", vec);
}

NOTF_CLOSE_NAMESPACE

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
        return format_to(ctx.begin(), "{}({}x{})", type::get_type_name(), size.width(), size.height());
    }
};

} // namespace fmt

// std::hash ======================================================================================================== //

/// std::hash specialization for notf::Size2.
template<class Element>
struct std::hash<notf::detail::Size2<Element>> {
    size_t operator()(const notf::detail::Size2<Element>& size2) const {
        return notf::hash(notf::to_number(notf::detail::HashID::SIZE2), size2.get_hash());
    }
};

// compile time tests =============================================================================================== //

static_assert(notf::is_pod_v<notf::Size2f>);
static_assert(notf::is_pod_v<notf::Size2d>);
static_assert(notf::is_pod_v<notf::Size2i>);
static_assert(notf::is_pod_v<notf::Size2s>);

static_assert(sizeof(notf::Size2f) == sizeof(float) * 2);
static_assert(sizeof(notf::Size2d) == sizeof(double) * 2);
static_assert(sizeof(notf::Size2i) == sizeof(int) * 2);
static_assert(sizeof(notf::Size2s) == sizeof(short) * 2);
