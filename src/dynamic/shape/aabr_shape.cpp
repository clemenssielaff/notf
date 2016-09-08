#include "dynamic/shape/aabr_shape.hpp"

#include "core/widget.hpp"

namespace signal {

Aabr AabrShapeComponent::get_aabr(const Widget& widget)
{
    // position
    Aabr aabr = m_aabr;
//    aabr.set_position(widget.get_position());

    // scale
//    aabr.set_width(aabr.width() * widget.get_scale().x);
//    aabr.set_height(aabr.height() * widget.get_scale().y);

    // no rotation

    return m_aabr;
}

void AabrShapeComponent::set_aabr(Aabr aabr)
{
    m_aabr = std::move(aabr);
    redraw_widgets();
}

} // namespace signal
