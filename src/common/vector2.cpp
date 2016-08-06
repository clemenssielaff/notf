#include "common/vector2.hpp"

#include <iostream>

namespace signal {

std::ostream& operator<<(std::ostream& out, const Vector2& vec)
{
    return out << "Vector2(" << vec.x << ", " << vec.y << ")";
}

} // namespace signal
