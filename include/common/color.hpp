#pragma once

#include <iosfwd>
#include <type_traits>

#include "int_utils.hpp"
#include "real.hpp"

namespace notf {

/// @brief The Color class.
struct Color {

    //  FIELDS  ///////////////////////////////////////////////////////////////////////////////////////////////////////

    /// @brief Red component in range [0, 1].
    float r;

    /// @brief Green component in range [0, 1].
    float g;

    /// @brief Blue component in range [0, 1].
    float b;

    /// @brief Alpha component in range [0, 1].
    float a;

    //  HOUSEHOLD  ////////////////////////////////////////////////////////////////////////////////////////////////////

    Color() = default; // required so this class remains a POD

    Color(float r, float g, float b, float a = 1)
        : r(clamp(r, 0, 1))
        , g(clamp(g, 0, 1))
        , b(clamp(b, 0, 1))
        , a(clamp(a, 0, 1))
    {
    }

    /** Creates a color from integer RGB values.
     * The parameters are clamped to range [0, 255] before cast to float.
     */
    template <class T, typename = std::enable_if_t<std::is_integral<T>::value>>
    Color(T r, T g, T b, T a = 255)
        : Color(static_cast<float>(clamp(r, 0, 255)) / 255.f,
                static_cast<float>(clamp(g, 0, 255)) / 255.f,
                static_cast<float>(clamp(b, 0, 255)) / 255.f,
                static_cast<float>(clamp(a, 0, 255)) / 255.f)
    {
    }

    static Color from_rgb(float r, float g, float b, float a = 1) { return Color(r, g, b, a); }

    template <class T, typename = std::enable_if_t<std::is_integral<T>::value>>
    static Color from_rgb(T r, T g, T b, T a = 1) { return Color(r, g, b, a); }

    static Color from_hsl(float h, float s, float l, float a = 1);
};

//  FREE FUNCTIONS  ///////////////////////////////////////////////////////////////////////////////////////////////////

/// @brief Prints the contents of this Color into a std::ostream.
///
/// @param os       Output stream, implicitly passed with the << operator.
/// @param color    Color to print.
///
/// @return Output stream for further output.
std::ostream& operator<<(std::ostream& out, const Color& color);

/** Linear interpolation between two Color%s.
 * @param from      Left Color, full weight at bend = 0.
 * @param to        Right Color, full weight at bend = 1.
 * @param blend     Blend value, clamped to range [0, 1].
 * @return          Interpolated Color.
 */
inline Color lerp(const Color& from, const Color& to, float blend)
{
    blend = clamp(blend, 0, 1);
    const float inv = 1.f - blend;
    return Color{
        (from.r * inv) + (to.r * blend),
        (from.g * inv) + (to.g * blend),
        (from.b * inv) + (to.b * blend),
        (from.a * inv) + (to.a * blend),
    };
}

} // namespace notf
