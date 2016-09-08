#include "dynamic/color/singlecolor.hpp"

namespace signal {

Color SingleColorComponent::get_color(int) const
{
    return m_color;
}

} // namespace signal
