#include "dynamic/layout/overlayout.hpp"

#include <algorithm>
#include <sstream>

#include "common/aabr.hpp"
#include "common/log.hpp"
#include "common/vector.hpp"
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
        std::stringstream ss;
        ss << "Encountered invalid padding: " << padding;
        const std::string msg = ss.str();
        log_warning << msg;
        throw std::runtime_error(std::move(msg));
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

bool Overlayout::get_widgets_at(const Vector2f local_pos, std::vector<Widget*>& result)
{
    bool found_any = false;
    for (Item* item : m_items) {
        if (LayoutItem* layout_item = item->get_layout_item()) {
            // TODO: Overlayout::get_widget_at does not respect transform (only translate)
            const Vector2f item_pos = local_pos - layout_item->get_transform().get_translation();
            const Aabrf item_rect(layout_item->get_size());
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
    const Size2f total_size     = get_size();
    const Size2f available_size = {total_size.width - m_padding.width(), total_size.height - m_padding.height()};
    for (Item* item : m_items) {
        if (LayoutItem* layout_item = item->get_layout_item()) {

            // the item's size is independent of its placement
            _set_item_size(layout_item, available_size);
            const Size2f item_size = layout_item->get_size();

            // the item's transform depends on the Overlayout's alignment
            float x;
            if (m_horizontal_alignment == Horizontal::LEFT) {
                x = m_padding.left;
            }
            else if (m_horizontal_alignment == Horizontal::CENTER) {
                x = ((available_size.width - item_size.width) / 2.f) + m_padding.left;
            }
            else {
                assert(m_horizontal_alignment == Horizontal::RIGHT);
                x = total_size.width - m_padding.right - item_size.width;
            }

            float y;
            if (m_vertical_alignment == Vertical::TOP) {
                y = m_padding.top;
            }
            else if (m_vertical_alignment == Vertical::CENTER) {
                y = ((available_size.height - item_size.height) / 2.f) + m_padding.top;
            }
            else {
                assert(m_vertical_alignment == Vertical::BOTTOM);
                y = total_size.height - m_padding.bottom - item_size.height;
            }
            _set_item_transform(layout_item, Xform2f::translation({x, y}));
        }
    }
}

} // namespace notf
