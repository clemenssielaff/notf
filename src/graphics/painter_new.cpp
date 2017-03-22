#include "graphics/painter_new.hpp"

#include "graphics/render_context.hpp"

namespace notf {

Paint Paint::create_linear_gradient(const Vector2f& start_pos, const Vector2f& end_pos,
                                    const Color start_color, const Color end_color)
{
    static const float large_number = 1e5;

    Vector2f delta  = end_pos - start_pos;
    const float mag = delta.magnitude();
    if (mag == approx(0., 0.0001)) {
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
    paint.xform[2][1]   = start_pos.y - (delta.y * large_number);
    paint.radius        = 0.0f;
    paint.feather       = max(1.0f, mag);
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
    paint.feather       = max(1.f, outer_radius - inner_radius);
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
    paint.xform         = Xform2f::translation({center.x + extend.width / 2, center.y + extend.height / 2});
    paint.radius        = radius;
    paint.feather       = max(1.f, feather);
    paint.extent.width  = extend.width / 2;
    paint.extent.height = extend.height / 2;
    paint.inner_color   = std::move(inner_color);
    paint.outer_color   = std::move(outer_color);
    return paint;
}

Paint Paint::create_texture_pattern(const Vector2f& top_left, const Size2f& extend,
                                    std::shared_ptr<Texture2> texture,
                                    const float angle, const float alpha)
{
    Paint paint;
    paint.xform         = Xform2f::rotation(angle);
    paint.xform[2][0]   = top_left.x;
    paint.xform[2][1]   = top_left.y;
    paint.extent.width  = extend.width;
    paint.extent.height = extend.height;
    paint.texture       = texture;
    paint.inner_color   = Color(1, 1, 1, alpha);
    paint.outer_color   = Color(1, 1, 1, alpha);
    return paint;
}

/**********************************************************************************************************************/

std::vector<float> Painter::s_commands;

Painter::Painter(Cell& cell, const RenderContext& context)
    : m_cell(cell)
    , m_states({State()})
    , m_tesselation_tolerance(0.25f / context.get_pixel_ratio())
    , m_distance_tolerance(0.01f / context.get_pixel_ratio())
    , m_fringe_width(1.f / context.get_pixel_ratio())
{
    s_commands.clear();
    //    m_cell.reset();
}

size_t Painter::push_state()
{
    assert(!m_states.empty());
    m_states.emplace_back(m_states.back());
    return m_states.size() - 1;
}

size_t Painter::pop_state()
{
    if (m_states.size() > 1) {
        m_states.pop_back();
    }
    else {
        m_states.back() = State();
    }
    assert(!m_states.empty());
    return m_states.size() - 1;
}

void Painter::set_scissor(const Aabrf& aabr)
{
    State& current_state        = _get_current_state();
    current_state.scissor.xform = Xform2f::translation(aabr.center());
    current_state.scissor.xform *= current_state.xform;
    current_state.scissor.extend = aabr.extend();
}

} // namespace notf
