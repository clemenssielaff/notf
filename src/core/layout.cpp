#include "core/layout.hpp"

#include "core/item_container.hpp"

namespace notf {

Layout::Layout(ItemContainerPtr container)
    : ScreenItem(std::move(container))
    , m_has_explicit_claim(false)
{
}

bool Layout::set_claim(const Claim claim)
{
    if (claim.is_zero()) {
        m_has_explicit_claim = false;
        return _set_claim(_aggregate_claim());
    }
    else {
        m_has_explicit_claim = true;
        return _set_claim(std::move(claim));
    }
}

void Layout::clear()
{
    m_children->clear();
}

bool Layout::_set_size(const Size2f size)
{
    if (ScreenItem::_set_size(size)) {
        _relayout();
        on_layout_changed();
        return true;
    }
    return false;
}

bool Layout::_update_claim()
{
    if (m_has_explicit_claim) {
        return false;
    }
    return _set_claim(_aggregate_claim());
}

} // namespace notf
