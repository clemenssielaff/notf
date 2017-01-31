#pragma once

#include "common/vector2.hpp"
#include "graphics/hud_primitives.hpp"

namespace notf {

class HUDPainter {

public:


public: // static methods
    static Paint create_linear_gradient(const Vector2& start_pos, const Vector2& end_pos,
                                        const Color start_color, const Color end_color)
    {
        static const float large_number = 1e5;

        Vector2 delta   = end_pos - start_pos;
        const float mag = delta.magnitude();
        if (mag == approx(0, 0.0001f)) {
            delta.x = 0;
            delta.y = 1;
        }
        else {
            delta.x /= mag;
            delta.y /= mag;
        }

        Paint paint;
        paint.xform[0][0]   = delta.y;
        paint.xform[0][1]   = -delta.x;
        paint.xform[1][0]   = delta.x;
        paint.xform[1][1]   = delta.y;
        paint.xform[2][0]   = start_pos.x - (delta.x * large_number);
        paint.xform[2][2]   = start_pos.y - (delta.y * large_number);
        paint.radius        = 0.0f;
        paint.feather       = max(1.0f, mag);
        paint.extent.width  = large_number;
        paint.extent.height = large_number + (mag / 2);
        paint.innerColor    = std::move(start_color);
        paint.outerColor    = std::move(end_color);
        return paint;
    }

    static Paint create_radial_gradient(const Vector2& center,
                                        const float inner_radius, const float outer_radius,
                                        const Color inner_color, const Color outer_color)
    {
        Paint paint;
        paint.xform         = Transform2::translation(center);
        paint.radius        = (inner_radius + outer_radius) * 0.5f;
        paint.feather       = max(1.f, outer_radius - inner_radius);
        paint.extent.width  = paint.radius;
        paint.extent.height = paint.radius;
        paint.innerColor    = std::move(inner_color);
        paint.outerColor    = std::move(outer_color);
        return paint;
    }

    static Paint create_box_gradient(const Vector2& center, const Size2f& extend,
                                     const float radius, const float feather,
                                     const Color inner_color, const Color outer_color)
    {
        Paint paint;
        paint.xform         = Transform2::translation({center.x + extend.width / 2, center.y + extend.height / 2});
        paint.radius        = radius;
        paint.feather       = max(1.f, feather);
        paint.extent.width  = extend.width / 2;
        paint.extent.height = extend.height / 2;
        paint.innerColor    = std::move(inner_color);
        paint.outerColor    = std::move(outer_color);
        return paint;
    }
};

} // namespace notf
