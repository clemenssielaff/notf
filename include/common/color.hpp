#pragma once

#include <array>
#include <iosfwd>
#include <string>

#include "common/float.hpp"
#include "common/integer.hpp"
#include "utils/sfinae.hpp"

namespace notf {

//*********************************************************************************************************************/

/**
 * A Color, stored as RGBA floating-point values.
 */
struct Color {

    /* Fields *********************************************************************************************************/

    /** Red component in range [0, 1]. */
    float r;

    /** Green component in range [0, 1]. */
    float g;

    /** Blue component in range [0, 1]. */
    float b;

    /** Alpha component in range [0, 1]. */
    float a;

    /* Constructors ***************************************************************************************************/

    Color() = default; // required so this class remains a POD

    Color(const float r, const float g, const float b, const float a = 1)
        : r(clamp(r, 0, 1))
        , g(clamp(g, 0, 1))
        , b(clamp(b, 0, 1))
        , a(clamp(a, 0, 1))
    {
    }

    /** Creates a color from integer RGB(A) values.
     * The parameters are clamped to range [0, 255] before cast to float.
     */
    template <class Integer, ENABLE_IF_INT(Integer)>
    Color(const Integer r, const Integer g, const Integer b, const Integer a = 255)
        : Color(static_cast<float>(clamp(r, 0, 255)) / 255.f,
                static_cast<float>(clamp(g, 0, 255)) / 255.f,
                static_cast<float>(clamp(b, 0, 255)) / 255.f,
                static_cast<float>(clamp(a, 0, 255)) / 255.f)
    {
    }

    /** Creates a color from a color value string.
     * Valid formattings are "#0099aa", "#0099aaff", "0099aa" or "0099aaff", string is parsed case-insensitive.
     * @throws std::runtime_error   If the given string is not a valid value string.
     */
    Color(const std::string& value);

    /* Static Constructors ********************************************************************************************/

    /** Creates a Color from floating-point RGB(A) values in the range [0, 1]. */
    static Color from_rgb(const float r, const float g, const float b, const float a = 1) { return Color(r, g, b, a); }

    /** Creates a Color from integer RGB(A) values in the range [0, 255]. */
    template <class Integer, ENABLE_IF_INT(Integer)>
    static Color from_rgb(const Integer r, const Integer g, const Integer b, const Integer a = 255) { return Color(r, g, b, a); }

    /** Creates a new Color from HSL values.
     * @param h     Hue in radians, in the range [0, 2*pi)
     * @param s     Saturation in the range [0, 1]
     * @param l     Lightness in the range [0, 1]
     * @param a     Alpha in the range [0, 1]
     */
    static Color from_hsl(const float h, const float s, const float l, const float a = 1);

    /*  Inspection  ***************************************************************************************************/

    /** Checks, if the given string is a valid color value that can be passed to the constructor. */
    static bool is_color(const std::string& value);

    /** Operators *****************************************************************************************************/

    /** Tests whether two Colors are equal. */
    bool operator==(const Color& other) const
    {
        return (r == approx(other.r)
                && g == approx(other.g)
                && b == approx(other.b)
                && a == approx(other.a));
    }

    /** Tests whether two Colors are not equal. */
    bool operator!=(const Color& other) const
    {
        return (r != approx(other.r)
                || g != approx(other.g)
                || b != approx(other.b)
                || a != approx(other.a));
    }

    /** Modifiers *****************************************************************************************************/

    /** Returns the Color as an RGB string value. */
    std::string to_string() const;

    /** Weighted conversion of this Color to greyscale. */
    Color to_greyscale() const;

    /** Premultiplied copy of this Color. */
    Color premultiplied() const
    {
        return {r * a,
                g * a,
                b * a,
                a};
    }

    /** Allows direct memory (read / write) access to the Color's internal storage. */
    float* as_ptr() { return &r; }
};

/* FREE FUNCTIONS *****************************************************************************************************/

/** Linear interpolation between two Color%s.
 * @param from      Left Color, full weight at bend = 0.
 * @param to        Right Color, full weight at bend = 1.
 * @param blend     Blend value, clamped to range [0, 1].
 * @return          Interpolated Color.
 */
inline Color lerp(const Color& from, const Color& to, float blend)
{
    blend           = clamp(blend, 0, 1);
    const float inv = 1.f - blend;
    return Color{
        (from.r * inv) + (to.r * blend),
        (from.g * inv) + (to.g * blend),
        (from.b * inv) + (to.b * blend),
        (from.a * inv) + (to.a * blend),
    };
}

/** Prints the contents of this Color into a std::ostream.
 * @param out   Output stream, implicitly passed with the << operator.
 * @param color Color to print.
 * @return      Output stream for further output.
 */
std::ostream& operator<<(std::ostream& out, const Color& color);

} // namespace notf
