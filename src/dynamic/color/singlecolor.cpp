#include "dynamic/color/singlecolor.hpp"

namespace notf {

Color SingleColor::get_color(int) const
{
    return m_color;
}

} // namespace notf
