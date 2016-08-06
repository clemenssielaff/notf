#include "common/aabr.hpp"

#include <iostream>

namespace signal {

std::ostream& operator<<(std::ostream& out, const Aabr& aabr)
{
    return out << "Rect([" << aabr.left() << ", " << aabr.bottom() << "], ["
               << aabr.right() << ", " << aabr.top() << "])";
}

} // namespace signal
