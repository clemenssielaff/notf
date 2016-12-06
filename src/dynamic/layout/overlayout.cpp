#include "dynamic/layout/overlayout.hpp"

#include <algorithm>

#include "common/log.hpp"
#include "common/vector_utils.hpp"
#include "utils/make_smart_enabler.hpp"

namespace notf {

const LayoutItem* OverlayoutIterator::next()
{
    if (m_index >= m_layout->m_items.size()) {
        return nullptr;
    }
    return m_layout->m_items[m_index++];
}

void Overlayout::set_padding(const Padding& padding)
{
    if (!padding.is_valid()) {
        log_warning << "Ignoring invalid padding: " << padding;
        return;
    }
    if (padding != m_padding) {
        m_padding = padding;
        _relayout();
    }
}

void Overlayout::add_item(std::shared_ptr<LayoutItem> item)
{
    // if the item is already child of this Layout, place it at the end
    if (has_item(item)) {
        remove_one_unordered(m_items, item.get());
    }
    else {
        _add_child(item);
    }
    m_items.emplace_back(item.get());
    _relayout();
}

std::shared_ptr<Widget> Overlayout::get_widget_at(const Vector2& /*local_pos*/)
{
    return {};
}

std::unique_ptr<LayoutIterator> Overlayout::iter_items() const
{
    return std::make_unique<MakeSmartEnabler<OverlayoutIterator>>(this);
}

void Overlayout::_remove_item(const LayoutItem* item)
{
    auto it = std::find(m_items.begin(), m_items.end(), item);
    assert(it != m_items.end());
    m_items.erase(it);
}

void Overlayout::_relayout()
{
    const Size2f total_size = get_size();
    const Size2f effective_size = {total_size.width - m_padding.width(), total_size.height - m_padding.height()};
    for (LayoutItem* item : m_items) {
        _set_item_transform(item, Transform2::translation({m_padding.left, m_padding.top}));
        _set_item_size(item, std::move(effective_size));
    }
}

} // namespace notf
