#include "dynamic/layout/overlayout.hpp"

#include <algorithm>

#include "common/aabr.hpp"
#include "common/log.hpp"
#include "common/vector_utils.hpp"
#include "utils/make_smart_enabler.hpp"

namespace notf {

const Item* OverlayoutIterator::next()
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

void Overlayout::add_item(std::shared_ptr<Item> item)
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

bool Overlayout::get_widgets_at(const Vector2 local_pos, std::vector<Widget*>& result)
{
    bool found_any = false;
    for (Item* item : m_items) {
        if (LayoutItem* layout_item = item->get_layout_item()) {
            // TODO: Overlayout::get_widget_at does not respect transform (only translate)
            const Vector2 item_pos = local_pos - layout_item->get_transform().get_translation();
            const Aabr item_rect(layout_item->get_size());
            if (item_rect.contains(item_pos)) {
                found_any |= layout_item->get_widgets_at(item_pos, result);
            }
        }
    }
    return found_any;
}

std::unique_ptr<LayoutIterator> Overlayout::iter_items() const
{
    return std::make_unique<MakeSmartEnabler<OverlayoutIterator>>(this);
}

void Overlayout::_remove_item(const Item* item)
{
    auto it = std::find(m_items.begin(), m_items.end(), item);
    assert(it != m_items.end());
    m_items.erase(it);
}

void Overlayout::_relayout()
{
    const Size2f total_size = get_size();
    const Size2f effective_size = {total_size.width - m_padding.width(), total_size.height - m_padding.height()};
    for (Item* item : m_items) {
        if (LayoutItem* layout_item = item->get_layout_item()) {
            _set_item_transform(layout_item, Transform2::translation({m_padding.left, m_padding.top}));
            _set_item_size(layout_item, std::move(effective_size));
        }
    }
}

} // namespace notf
