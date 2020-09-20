#pragma once

#include <iosfwd>
#include <string>

#include "notf/meta/hash.hpp"
#include "notf/meta/real.hpp"

NOTF_OPEN_NAMESPACE

// color ============================================================================================================ //

/// A Color, stored as RGBA floating-point values.
struct Color {

    // methods --------------------------------------------------------------------------------- //
public:
    // constructors -----------------------------------------------------------

    /// Default (non-initializing) constructor so this struct remains a POD
    Color() = default;

    /// Value constructor.
    Color(const float r, const float g, const float b, const float a = 1)
        : r(clamp(r, 0, 1)), g(clamp(g, 0, 1)), b(clamp(b, 0, 1)), a(clamp(a, 0, 1)) {}

    /// Creates a color from integer RGB(A) values.
    /// The parameters are clamped to range [0, 255] before cast to float.
    template<class Integer, typename = std::enable_if_t<std::is_integral<Integer>::value>>
    Color(const Integer r, const Integer g, const Integer b, const Integer a = 255)
        : Color(static_cast<float>(clamp(r, 0, 255)) / 255.f, static_cast<float>(clamp(g, 0, 255)) / 255.f,
                static_cast<float>(clamp(b, 0, 255)) / 255.f, static_cast<float>(clamp(a, 0, 255)) / 255.f) {}

    /// Creates a color from a color value string.
    /// Valid formattings are "#0099aa", "#0099aaff", "0099aa" or "0099aaff", string is parsed case-insensitive.
    /// @throws ValueError  If the given string is not a valid value string.
    Color(const std::string& value);

    // static constructors ----------------------------------------------------

    /// Creates a Color from floating-point RGB(A) values in the range [0, 1].
    static Color from_rgb(const float r, const float g, const float b, const float a = 1) { return Color(r, g, b, a); }

    /// Creates a Color from integer RGB(A) values in the range [0, 255].
    template<class Integer, typename = std::enable_if_t<std::is_integral<Integer>::value>>
    static Color from_rgb(const Integer r, const Integer g, const Integer b, const Integer a = 255) {
        return Color(r, g, b, a);
    }

    /// Creates a new Color from HSL values.
    /// @param h     Hue in radians, in the range [0, 2*pi)
    /// @param s     Saturation in the range [0, 1]
    /// @param l     Lightness in the range [0, 1]
    /// @param a     Alpha in the range [0, 1]
    static Color from_hsl(const float h, const float s, const float l, const float a = 1);

    /// @{
    /// Colors.
    static Color transparent() { return Color(0., 0., 0., 0.); }
    static Color black() { return Color(0., 0., 0., 1.); }
    static Color white() { return Color(1., 1., 1., 1.); }
    static Color grey() { return Color(0.5, 0.5, 0.5, 1.); }
    static Color red() { return Color(1., 0., 0., 1.); }
    static Color green() { return Color(0., 1., 0., 1.); }
    static Color blue() { return Color(0., 0., 1., 1.); }
    /// @}

    // inspection -------------------------------------------------------------

    /// Checks, if the given string is a valid color value that can be passed to the constructor.
    static bool is_color(const std::string& string);

    // operators --------------------------------------------------------------

    /// Tests if two Colors are approximately equal.
    constexpr bool is_approx(const Color& other, const float epsilon = precision_high<float>()) const noexcept {
        return ::notf::is_approx(r, other.r, epsilon)    //
               && ::notf::is_approx(g, other.g, epsilon) //
               && ::notf::is_approx(b, other.b, epsilon) //
               && ::notf::is_approx(a, other.a, epsilon);
    }

    /// Tests whether two Colors are equal.
    constexpr bool operator==(const Color& other) const noexcept { return is_approx(other); }

    /// Tests whether two Colors are not equal.
    constexpr bool operator!=(const Color& other) const noexcept { return !(*this == other); }

    // modifiers --------------------------------------------------------------

    /// Returns the Color as an RGB string value.
    std::string to_string() const;

    /// Weighted conversion of this Color to greyscale.
    Color to_greyscale() const;

    /// Color without any transparency.
    Color to_opaque() const { return {r, g, b, 1}; }

    /// Premultiplied copy of this Color.
    Color premultiplied() const { return {r * a, g * a, b * a, a}; }

    /// Allows direct memory (read / write) access to the Color's internal storage.
    float* as_ptr() { return &r; }

    // fields ---------------------------------------------------------------------------------- //
public:
    /// Red component in range [0, 1].
    float r;

    /// Green component in range [0, 1].
    float g;

    /// Blue component in range [0, 1].
    float b;

    /// Alpha component in range [0, 1].
    float a;
};

// free functions =================================================================================================== //

/// Linear interpolation between two Colors.
/// @param from      Left Color, full weight at bend = 0.
/// @param to        Right Color, full weight at bend = 1.
/// @param blend     Blend value, clamped to range [0, 1].
/// @return          Interpolated Color.
inline Color lerp(const Color& from, const Color& to, float blend) {
    blend = clamp(blend, 0, 1);
    const float inv = 1.f - blend;
    return Color{
        (from.r * inv) + (to.r * blend),
        (from.g * inv) + (to.g * blend),
        (from.b * inv) + (to.b * blend),
        (from.a * inv) + (to.a * blend),
    };
}

/// Prints the contents of this Color into a std::ostream.
/// @param out   Output stream, implicitly passed with the << operator.
/// @param color Color to print.
/// @return      Output stream for further output.
std::ostream& operator<<(std::ostream& out, const Color& color);

NOTF_CLOSE_NAMESPACE

// std::hash ======================================================================================================== //

/// std::hash specialization for notf::Color.
template<>
struct std::hash<notf::Color> {
    size_t operator()(const notf::Color& color) const {
        return notf::hash(static_cast<size_t>(notf::detail::HashID::COLOR), color.r, color.g, color.b, color.a);
    }
};
