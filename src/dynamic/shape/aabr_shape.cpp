#include "dynamic/shape/aabr_shape.hpp"

#include "core/widget.hpp"

namespace notf {

Aabr RectShape::get_screen_aabr(const Widget& /*widget*/)
{
    // position
    Aabr aabr;
    //    aabr.set_position(widget.get_position());

    // scale
    //    aabr.set_width(aabr.width() * widget.get_scale().x);
    //    aabr.set_height(aabr.height() * widget.get_scale().y);

    // no rotation

    return aabr;
}

void RectShape::set_fixed()
{
    set_horizontal_size({get_horizontal_size().get_preferred()});
    set_vertical_size({get_vertical_size().get_preferred()});
    redraw_widgets();
}

void RectShape::set_min_width(const Real width)
{
    set_horizontal_size(Stretch(get_horizontal_size().get_preferred(), width, get_horizontal_size().get_max()));
    redraw_widgets();
}

void RectShape::set_max_width(const Real width)
{
    set_horizontal_size(Stretch(get_horizontal_size().get_preferred(), get_horizontal_size().get_min(), width));
    redraw_widgets();
}

void RectShape::set_min_height(const Real height)
{
    set_vertical_size(Stretch(get_vertical_size().get_preferred(), height, get_vertical_size().get_max()));
    redraw_widgets();
}

void RectShape::set_max_height(const Real height)
{
    set_vertical_size(Stretch(get_vertical_size().get_preferred(), get_vertical_size().get_min(), height));
    redraw_widgets();
}

} // namespace notf
