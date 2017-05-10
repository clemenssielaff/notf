#pragma once

#include "common/color.hpp"
#include "common/size2.hpp"
#include "common/xform2.hpp"

namespace notf {

class Texture2;

/**********************************************************************************************************************/

/** Paint is an structure holding information about a particular draw Call.
 * Most of the paint fields are used to initialize the Fragment uniform in the Cell shader.
 */
struct Paint {
    /** Default Constructor. */
    Paint() = default;

    /** Value Constructor with a single Color. */
    Paint(Color color)
        : xform(Xform2f::identity())
        , extent()
        , radius(0)
        , feather(1)
        , inner_color(std::move(color))
        , outer_color(inner_color)
        , texture()
    {
    }

public: // static methods
    static Paint create_linear_gradient(const Vector2f& start_pos, const Vector2f& end_pos,
                                        const Color start_color, const Color end_color);

    static Paint create_radial_gradient(const Vector2f& center,
                                        const float inner_radius, const float outer_radius,
                                        const Color inner_color, const Color outer_color);

    static Paint create_box_gradient(const Vector2f& center, const Size2f& extend,
                                     const float radius, const float feather,
                                     const Color inner_color, const Color outer_color);

    static Paint create_texture_pattern(const Vector2f& origin, const Size2f& extend,
                                        std::shared_ptr<Texture2> texture,
                                        const float angle, const float alpha);

    /** Turns the Paint into a single solid. */
    void set_color(const Color color)
    {
        xform       = Xform2f::identity();
        radius      = 0;
        feather     = 1;
        inner_color = std::move(color);
        outer_color = inner_color;
    }

    /** Local transform of the Paint. */
    Xform2f xform;

    /** Extend of the Paint. */
    Size2f extent;

    float radius;

    float feather;

    /** Inner color of the gradient. */
    Color inner_color;

    /** Outer color of the gradient. */
    Color outer_color;

    /** Texture used within this Paint, can be empty. */
    std::shared_ptr<Texture2> texture;
};

} // namespace notf
