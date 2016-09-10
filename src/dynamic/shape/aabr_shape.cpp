#include "dynamic/shape/aabr_shape.hpp"

#include "core/widget.hpp"

namespace signal {

Aabr RectShapeComponent::get_screen_aabr(const Widget& widget)
{
    // position
    Aabr aabr;
    //    aabr.set_position(widget.get_position());

    // scale
    //    aabr.set_width(aabr.width() * widget.get_scale().x);
    //    aabr.set_height(aabr.height() * widget.get_scale().y);

    // no rotation

    return m_aabr;
}

void RectShapeComponent::set_fixed()
{
    set_horizontal_size({get_horizontal_size().preferred});
    set_vertical_size({get_vertical_size().preferred});
    redraw_widgets();
}

void RectShapeComponent::set_min_width(const Real width)
{
    set_horizontal_size(SizeRange(get_horizontal_size().preferred, width, get_horizontal_size().max));
    redraw_widgets();
}

void RectShapeComponent::set_max_width(const Real width)
{
    set_horizontal_size(SizeRange(get_horizontal_size().preferred, get_horizontal_size().min, width));
    redraw_widgets();
}

void RectShapeComponent::set_min_height(const Real height)
{
    set_vertical_size(SizeRange(get_vertical_size().preferred, height, get_vertical_size().max));
    redraw_widgets();
}

void RectShapeComponent::set_max_height(const Real height)
{
    set_vertical_size(SizeRange(get_vertical_size().preferred, get_vertical_size().min, height));
    redraw_widgets();
}

} // namespace signal
