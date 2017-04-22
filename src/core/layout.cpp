#include "core/layout.hpp"

namespace notf {

Layout::Layout()
    : ScreenItem()
    , m_override_claim(false)
{
}

bool Layout::set_claim(const Claim claim)
{
    if (claim.is_zero()) {
        m_override_claim = false;
        return _set_claim(_aggregate_claim());
    }
    else {
        m_override_claim = true;
        return _set_claim(std::move(claim));
    }
}

bool Layout::_set_size(const Size2f size)
{
    if (ScreenItem::_set_size(std::move(size))) {
        _relayout();
        return true;
    }
    return false;
}

bool Layout::_update_claim()
{
    if (m_override_claim) {
        return false;
    }
    return _set_claim(_aggregate_claim());
}

} // namespace notf
