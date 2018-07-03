#pragma once

#include "common/color.hpp"
#include "common/matrix3.hpp"
#include "common/size2.hpp"
#include "graphics/forwards.hpp"

NOTF_OPEN_NAMESPACE

// ================================================================================================================== //

/// Paint is an structure holding information about a particular draw call.
/// Most of the paint fields are used to initialize the fragment uniforms in the Plotter's shader.
class Paint {

    // types -------------------------------------------------------------------------------------------------------- //
public:
    /// Type of cap used at the end of a painted line.
    enum class LineCap : unsigned char {
        BUTT,
        ROUND,
        SQUARE,
    };

    /// Type of joint beween two painted line segments.
    enum class LineJoin : unsigned char {
        MITER,
        ROUND,
        BEVEL,
    };

    /// Winding direction of a painted Shape.
    enum class Winding : unsigned char {
        CCW,
        CW,
        COUNTERCLOCKWISE = CCW,
        CLOCKWISE = CW,
        SOLID = CCW,
        HOLE = CW,
    };

    // methods ------------------------------------------------------------------------------------------------------ //
public:
    /// Default Constructor.
    Paint() = default;

    /// Value Constructor.
    /// @param color    Single color to paint.
    Paint(Color color) : inner_color(std::move(color)), outer_color(inner_color) {}

    /// Creates a linear gradient.
    static Paint
    linear_gradient(const Vector2f& start_pos, const Vector2f& end_pos, Color start_color, Color end_color);

    static Paint radial_gradient(const Vector2f& center, float inner_radius, float outer_radius, Color inner_color,
                                 Color outer_color);

    static Paint box_gradient(const Vector2f& center, const Size2f& extend, float radius, float feather,
                              Color inner_color, Color outer_color);

    static Paint
    texture_pattern(const Vector2f& origin, const Size2f& extend, TexturePtr texture, float angle, float alpha);

    /// Turns the Paint into a single solid color.
    void set_color(Color color)
    {
        xform = Matrix3f::identity();
        radius = 0;
        feather = 1;
        inner_color = std::move(color);
        outer_color = inner_color;
    }

    // fields ------------------------------------------------------------------------------------------------------- //
public:
    /// Local transform of the Paint.
    Matrix3f xform = Matrix3f::identity();

    /// Texture used within this Paint, can be empty.
    TexturePtr texture;

    /// Inner gradient color.
    Color inner_color = Color::black();

    /// Outer gradient color.
    Color outer_color = Color::black();

    /// Extend of the Paint.
    Size2f extent = Size2f::zero();

    float radius = 0;

    float feather = 1;
};

NOTF_CLOSE_NAMESPACE
