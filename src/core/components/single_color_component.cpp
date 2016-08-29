#include "core/components/single_color_component.hpp"

namespace signal {

Color SingleColorComponent::get_color(int) const
{
    return m_color;
}

} // namespace signal
