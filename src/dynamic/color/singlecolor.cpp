#include "dynamic/color/singlecolor.hpp"

namespace signal {

Color SingleColor::get_color(int) const
{
    return m_color;
}

} // namespace signal
