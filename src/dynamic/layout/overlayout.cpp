#include "dynamic/layout/overlayout.hpp"

#include <algorithm>
#include <sstream>

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

void Overlayout::set_alignment(Alignment alignment)
{
    // check horizontal
    if (alignment & Alignment::LEFT) {
        if (alignment & (Alignment::RIGHT | Alignment::HCENTER)) {
            goto error;
        }
    }
    else if (alignment & Alignment::RIGHT) {
        if (alignment & Alignment::HCENTER) {
            goto error;
        }
    }
    else if (!(alignment & Alignment::HCENTER)) {
        alignment = static_cast<Alignment>(alignment | Alignment::LEFT); // use LEFT if nothing is set
    }

    // check vertical
    if (alignment & Alignment::TOP) {
        if (alignment & (Alignment::BOTTOM | Alignment::VCENTER)) {
            goto error;
        }
    }
    else if (alignment & Alignment::BOTTOM) {
        if (alignment & Alignment::VCENTER) {
            goto error;
        }
    }
    else if (!(alignment & Alignment::VCENTER)) {
        alignment = static_cast<Alignment>(alignment | Alignment::TOP); // use TOP if nothing is set
    }

    m_alignment = alignment;
    return;

error:
    std::stringstream ss;
    ss << "Encountered invalid alignment: " << alignment;
    const std::string msg = ss.str();
    log_warning << msg;
    throw std::runtime_error(std::move(msg));
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
    const Size2f total_size     = get_size();
    const Size2f available_size = {total_size.width - m_padding.width(), total_size.height - m_padding.height()};
    for (Item* item : m_items) {
        if (LayoutItem* layout_item = item->get_layout_item()) {

            // the item's size is independent of its placement
            const Claim& claim               = layout_item->get_claim();
            const Claim::Stretch& horizontal = claim.get_horizontal();
            const Claim::Stretch& vertical   = claim.get_vertical();
            const Size2f item_size           = {
                max(horizontal.get_min(), min(horizontal.get_max(), available_size.width)),
                max(vertical.get_min(), min(vertical.get_max(), available_size.height))};
            _set_item_size(layout_item, item_size);

            // the item's transform depends on the Overlayout's alignment
            float x, y;
            if (m_alignment & Alignment::LEFT) {
                x = m_padding.left;
            }
            else if (m_alignment & Alignment::HCENTER) {
                x = ((available_size.width - item_size.width) / 2.f) + m_padding.left;
            }
            else {
                assert(m_alignment & Alignment::RIGHT);
                x = m_padding.right - item_size.width;
            }
            if (m_alignment & Alignment::TOP) {
                y = m_padding.top;
            }
            else if (m_alignment & Alignment::HCENTER) {
                y = ((available_size.height - item_size.height) / 2.f) + m_padding.top;
            }
            else {
                assert(m_alignment & Alignment::BOTTOM);
                y = m_padding.bottom - item_size.height;
            }
            _set_item_transform(layout_item, Transform2::translation({x, y}));
        }
    }
}

} // namespace notf
