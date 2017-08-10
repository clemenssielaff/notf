#include "core/layout.hpp"

#include "core/item_container.hpp"

namespace notf {

Layout::Layout(ItemContainerPtr container)
    : ScreenItem(std::move(container))
    , m_has_explicit_claim(false)
    , m_child_aabr(Aabrf::zero())
{
}

bool Layout::set_claim(const Claim claim)
{
    m_has_explicit_claim = true;
    return _set_claim(std::move(claim));
}

bool Layout::unset_claim()
{
    m_has_explicit_claim = false;
    return _set_claim(_consolidate_claim());
}

void Layout::clear()
{
    m_children->clear();
}

bool Layout::_update_claim()
{
    if (m_has_explicit_claim) {
        return false;
    }
    return _set_claim(_consolidate_claim());
}

} // namespace notf
