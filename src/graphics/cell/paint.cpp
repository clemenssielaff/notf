#include "graphics/cell/paint.hpp"

namespace notf {

Paint Paint::create_linear_gradient(const Vector2f& start_pos, const Vector2f& end_pos,
                                    const Color start_color, const Color end_color)
{
    static const float large_number = 1e5;

    Vector2f delta  = end_pos - start_pos;
    const float mag = delta.magnitude();
    if (mag == approx(0., 0.0001)) {
        delta.x() = 0;
        delta.y() = 1;
    }
    else {
        delta.x() /= mag;
        delta.y() /= mag;
    }

    Paint paint;
    paint.xform[0][0]   = delta.y();
    paint.xform[0][1]   = -delta.x();
    paint.xform[1][0]   = delta.x();
    paint.xform[1][1]   = delta.y();
    paint.xform[2][0]   = start_pos.x() - (delta.x() * large_number);
    paint.xform[2][1]   = start_pos.y() - (delta.y() * large_number);
    paint.radius        = 0.0f;
    paint.feather       = max(1, mag);
    paint.extent.width  = large_number;
    paint.extent.height = large_number + (mag / 2);
    paint.inner_color   = std::move(start_color);
    paint.outer_color   = std::move(end_color);
    return paint;
}

Paint Paint::create_radial_gradient(const Vector2f& center,
                                    const float inner_radius, const float outer_radius,
                                    const Color inner_color, const Color outer_color)
{
    Paint paint;
    paint.xform         = Xform2f::translation(center);
    paint.radius        = (inner_radius + outer_radius) * 0.5f;
    paint.feather       = max(1, outer_radius - inner_radius);
    paint.extent.width  = paint.radius;
    paint.extent.height = paint.radius;
    paint.inner_color   = std::move(inner_color);
    paint.outer_color   = std::move(outer_color);
    return paint;
}

Paint Paint::create_box_gradient(const Vector2f& center, const Size2f& extend,
                                 const float radius, const float feather,
                                 const Color inner_color, const Color outer_color)
{
    Paint paint;
    paint.xform         = Xform2f::translation({center.x() + extend.width / 2, center.y() + extend.height / 2});
    paint.radius        = radius;
    paint.feather       = max(1, feather);
    paint.extent.width  = extend.width / 2;
    paint.extent.height = extend.height / 2;
    paint.inner_color   = std::move(inner_color);
    paint.outer_color   = std::move(outer_color);
    return paint;
}

Paint Paint::create_texture_pattern(const Vector2f& origin, const Size2f& extend,
                                    std::shared_ptr<Texture2> texture,
                                    const float angle, const float alpha)
{
    Paint paint;
    paint.xform         = Xform2f::rotation(angle);
    paint.xform[2][0]   = origin.x();
    paint.xform[2][1]   = origin.y();
    paint.extent.width  = extend.width;
    paint.extent.height = -extend.height;
    paint.texture       = texture;
    paint.inner_color   = Color(1, 1, 1, alpha);
    paint.outer_color   = Color(1, 1, 1, alpha);
    return paint;
}

} // namespace notf
