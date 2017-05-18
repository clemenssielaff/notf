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

void Layout::_cascade_render_layer(const RenderLayerPtr& render_layer)
{
    if(_has_own_render_layer() || render_layer == m_render_layer){
        return;
    }
    m_render_layer = render_layer;
    auto it = iter_items();
    while (Item* item = it->next()) {
        Item::_cascade_render_layer(item, render_layer);
    }
}

bool Layout::_update_claim()
{
    if (m_override_claim) {
        return false;
    }
    return _set_claim(_aggregate_claim());
}

} // namespace notf
