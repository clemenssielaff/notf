/*
 * RGB <-> LAB and RGB <-> HSL conversion code adapted from:
 * https://github.com/davisking/dlib/blob/master/dlib/pixel.h
 * See thirdparty/dlib_license.txt for the license.
 */

#include <iostream>
#include <regex>
#include <sstream>
#include <type_traits>

#include "common/color.hpp"
#include "common/log.hpp"
#include "common/string.hpp"

namespace notf {

#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wfloat-equal"
#endif

/** Color values in HSL color space (must be cast to `Color` before you can work with them). */
struct Hsl {
    float h;
    float s;
    float l;
    float alpha;
};

/** Color values in LAB color space (must be cast to `Color` before you can work with them). */
struct Lab {
    float l;
    float a;
    float b;
    float alpha;
};

Hsl rgb_to_hsl(const Color& input)
{
    float themin, themax, delta;
    Hsl result;

    themin   = std::min(input.r, std::min(input.g, input.b));
    themax   = std::max(input.r, std::max(input.g, input.b));
    delta    = themax - themin;
    result.l = (themin + themax) / 2;
    result.s = 0;
    if (result.l > 0 && result.l < 1) {
        result.s = delta / (result.l < 0.5f ? (2 * result.l) : (2 - 2 * result.l));
    }
    result.h = 0;
    if (delta > 0) {
        if (themax == input.r && themax != input.g) {
            result.h += (input.g - input.b) / delta;
        }
        if (themax == input.g && themax != input.b) {
            result.h += (2 + (input.b - input.r) / delta);
        }
        if (themax == input.b && themax != input.r) {
            result.h += (4 + (input.r - input.g) / delta);
        }
        result.h *= 60;
    }
    result.alpha = input.a;
    return (result);
}

Color hsl_to_rgb(const Hsl& input)
{
    static const float _60deg  = static_cast<float>(PI / 3.l);
    static const float _120deg = static_cast<float>(TWO_PI * (1.l / 3.l));
    static const float _240deg = static_cast<float>(TWO_PI * (2.l / 3.l));
    static const float _360deg = static_cast<float>(TWO_PI);

    float r, g, b;
    if (input.h < _120deg) {
        r = (_120deg - input.h) / _60deg;
        g = input.h / _60deg;
        b = 0;
    }
    else if (input.h < _240deg) {
        r = 0;
        g = (_240deg - input.h) / _60deg;
        b = (input.h - _120deg) / _60deg;
    }
    else {
        r = (input.h - _240deg) / _60deg;
        g = 0;
        b = (_360deg - input.h) / _60deg;
    }

    r = std::min(r, 1.f);
    g = std::min(g, 1.f);
    b = std::min(b, 1.f);

    r = 2 * input.s * r + (1 - input.s);
    g = 2 * input.s * g + (1 - input.s);
    b = 2 * input.s * b + (1 - input.s);

    Color result;
    if (input.l < .5f) {
        result.r = input.l * r;
        result.g = input.l * g;
        result.b = input.l * b;
    }
    else {
        result.r = (1 - input.l) * r + 2 * input.l - 1;
        result.g = (1 - input.l) * g + 2 * input.l - 1;
        result.b = (1 - input.l) * b + 2 * input.l - 1;
    }
    result.a = input.alpha;

    return (result);
}

Lab rgb_to_lab(const Color& input)
{
    float var_R = input.r;
    float var_G = input.g;
    float var_B = input.b;

    if (var_R > 0.04045f) {
        var_R = powf(((var_R + 0.055f) / 1.055f), 2.4f);
    }
    else {
        var_R = var_R / 12.92f;
    }

    if (var_G > 0.04045f) {
        var_G = powf(((var_G + 0.055f) / 1.055f), 2.4f);
    }
    else {
        var_G = var_G / 12.92f;
    }

    if (var_B > 0.04045f) {
        var_B = powf(((var_B + 0.055f) / 1.055f), 2.4f);
    }
    else {
        var_B = var_B / 12.92f;
    }

    var_R = var_R * 100;
    var_G = var_G * 100;
    var_B = var_B * 100;

    //Observer = 2°, Illuminant = D65
    float X = var_R * 0.4124f + var_G * 0.3576f + var_B * 0.1805f;
    float Y = var_R * 0.2126f + var_G * 0.7152f + var_B * 0.0722f;
    float Z = var_R * 0.0193f + var_G * 0.1192f + var_B * 0.9505f;

    float var_X = X / 95.047f;
    float var_Y = Y / 100.f;
    float var_Z = Z / 108.883f;

    if (var_X > 0.008856f) {
        var_X = powf(var_X, (1.f / 3.f));
    }
    else {
        var_X = (7.787f * var_X) + (16.f / 116.f);
    }

    if (var_Y > 0.008856f) {
        var_Y = powf(var_Y, (1.f / 3.f));
    }
    else {
        var_Y = (7.787f * var_Y) + (16.f / 116.f);
    }

    if (var_Z > 0.008856f) {
        var_Z = powf(var_Z, (1.f / 3.f));
    }
    else {
        var_Z = (7.787f * var_Z) + (16.f / 116.f);
    }

    Lab result;
    result.l     = max(0.f, (116.f * var_Y) - 16);
    result.a     = max(-128.f, min(127.f, 500.f * (var_X - var_Y)));
    result.b     = max(-128.f, min(127.f, 200.f * (var_Y - var_Z)));
    result.alpha = input.a;
    return result;
}

Color lab_to_rgb(const Lab& input)
{
    float var_Y = (input.l + 16.f) / 116.f;
    float var_X = (input.a / 500.f) + var_Y;
    float var_Z = var_Y - (input.b / 200.f);

    if (powf(var_Y, 3) > 0.008856f) {
        var_Y = powf(var_Y, 3);
    }
    else {
        var_Y = (var_Y - 16.f / 116.f) / 7.787f;
    }

    if (powf(var_X, 3) > 0.008856f) {
        var_X = powf(var_X, 3);
    }
    else {
        var_X = (var_X - 16.f / 116.f) / 7.787f;
    }

    if (powf(var_Z, 3) > 0.008856f) {
        var_Z = powf(var_Z, 3);
    }
    else {
        var_Z = (var_Z - 16.f / 116.f) / 7.787f;
    }

    float X = var_X * 95.047f;
    float Y = var_Y * 100.f;
    float Z = var_Z * 108.883f;

    var_X = X / 100.f;
    var_Y = Y / 100.f;
    var_Z = Z / 100.f;

    float var_R = var_X * 3.2406f + var_Y * -1.5372f + var_Z * -0.4986f;
    float var_G = var_X * -0.9689f + var_Y * 1.8758f + var_Z * 0.0415f;
    float var_B = var_X * 0.0557f + var_Y * -0.2040f + var_Z * 1.0570f;

    if (var_R > 0.0031308f) {
        var_R = 1.055f * powf(var_R, (1 / 2.4f)) - 0.055f;
    }
    else {
        var_R = 12.92f * var_R;
    }

    if (var_G > 0.0031308f) {
        var_G = 1.055f * powf(var_G, (1 / 2.4f)) - 0.055f;
    }
    else {
        var_G = 12.92f * var_G;
    }

    if (var_B > 0.0031308f) {
        var_B = 1.055f * powf(var_B, (1 / 2.4f)) - 0.055f;
    }
    else {
        var_B = 12.92f * var_B;
    }

    Color result;
    result.r = max(0.f, min(1.f, var_R));
    result.g = max(0.f, min(1.f, var_G));
    result.b = max(0.f, min(1.f, var_B));
    result.a = input.alpha;
    return (result);
}

Color::Color(const std::string& value)
{
    if (!is_color(value)) {
        const std::string message = string_format("\"%s\" is not a valid color value", value.c_str());
        log_critical << message;
        throw std::runtime_error(std::move(message));
    }

    const size_t start_index = value.at(0) == '#' ? 1 : 0;
    {
        int val_r, val_g, val_b;
        std::istringstream(value.substr(start_index, 2)) >> std::hex >> val_r;
        std::istringstream(value.substr(start_index + 2, 2)) >> std::hex >> val_g;
        std::istringstream(value.substr(start_index + 4, 2)) >> std::hex >> val_b;
        r = static_cast<float>(val_r) / 255.f;
        g = static_cast<float>(val_g) / 255.f;
        b = static_cast<float>(val_b) / 255.f;
    }

    if (value.size() == start_index + 8) {
        int val_a;
        std::istringstream(value.substr(start_index + 6, 2)) >> std::hex >> val_a;
        a = static_cast<float>(val_a) / 255.f;
    }
    else {
        a = 1.f;
    }
}

Color Color::from_hsl(const float h, const float s, const float l, const float a)
{
    return hsl_to_rgb({norm_angle(h), s, l, a});
}

bool Color::is_color(const std::string& value)
{
    static const std::regex rgb_regex("^#?(?:[0-9a-fA-F]{2}){3,4}$");
    return std::regex_search(value, rgb_regex);
}

std::string Color::to_string() const
{
    std::ostringstream buffer;
    buffer << "#" << std::hex
           << static_cast<int>(roundf(r * 255.f))
           << static_cast<int>(roundf(g * 255.f))
           << static_cast<int>(roundf(b * 255.f))
           << static_cast<int>(roundf(a * 255.f));
    return buffer.str();
}

Color Color::to_greyscale() const
{
    // values from:
    // https://en.wikipedia.org/wiki/Grayscale#Colorimetric_.28luminance-preserving.29_conversion_to_grayscale
    const float grey = (r * 0.2126f) + (g * 0.7152f) + (b * 0.0722f);
    return {grey, grey, grey, a};
}

std::ostream& operator<<(std::ostream& out, const Color& color)
{
    return out << "Color(" << color.r << ", " << color.g << ", " << color.b << ", " << color.a << ")";
}

/**
 * Compile-time sanity check.
 */
static_assert(sizeof(Color) == sizeof(float) * 4,
              "This compiler seems to inject padding bits into the notf::Color memory layout. "
              "You should use compiler-specific #pragmas to enforce a contiguous memory layout.");

static_assert(std::is_pod<Color>::value,
              "This compiler does not recognize notf::Color as a POD.");

#ifdef __clang__
#pragma clang diagnostic pop
#endif

} // namespace notf
