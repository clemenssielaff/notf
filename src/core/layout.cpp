#include "core/layout.hpp"

namespace notf {

bool Layout::_set_size(const Size2f size)
{
    if (ScreenItem::_set_size(std::move(size))) {
        _relayout();
        return true;
    }
    return false;
}

} // namespace notf
