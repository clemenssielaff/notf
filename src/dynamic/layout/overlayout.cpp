#include "dynamic/layout/overlayout.hpp"

#include <algorithm>
#include <sstream>

#include "common/aabr.hpp"
#include "common/log.hpp"
#include "common/vector.hpp"
#include "core/controller.hpp"
#include "core/screen_item.hpp"
#include "utils/reverse_iterator.hpp"

namespace notf {

Item* OverlayoutIterator::next()
{
    if (m_index >= m_layout->m_items.size()) {
        return nullptr;
    }
    return m_layout->m_items[m_index++].get();
}

/**********************************************************************************************************************/

std::shared_ptr<Overlayout> Overlayout::create()
{
    struct make_shared_enabler : public Overlayout {
        make_shared_enabler()
            : Overlayout() {}
    };
    return std::make_shared<make_shared_enabler>();
}

Overlayout::Overlayout()
    : Layout()
    , m_padding(Padding::none())
    , m_horizontal_alignment(Horizontal::CENTER)
    , m_vertical_alignment(Vertical::CENTER)
    , m_items()
{
}

bool Overlayout::has_item(const ItemPtr& item) const
{
    return std::find(m_items.cbegin(), m_items.cbegin(), item) != m_items.cend();
}

void Overlayout::clear()
{
    for (ItemPtr& item : m_items) {
        on_child_removed(item->get_id());
    }
    m_items.clear();
}

void Overlayout::add_item(ItemPtr item)
{
    if (!item) {
        log_warning << "Cannot add an empty Item pointer to a Layout";
        return;
    }

    // if the item is already child of this Layout, place it at the end
    if (has_item(item)) {
        remove_one_unordered(m_items, item);
    }

    // Controllers are initialized the first time they are parented to a Layout
    if (Controller* controller = dynamic_cast<Controller*>(item.get())) {
        controller->initialize();

        // update the item's size to this one
        _set_size(controller->get_root_item().get(), get_size());
        // TODO: all of this should be a Layout function! ... but can it be?
    }
    else if (ScreenItem* screen_item = dynamic_cast<ScreenItem*>(item.get())) {
        _set_size(screen_item, get_size());
    }

    // take ownership of the Item
    _set_parent(item.get(), shared_from_this());
    const ItemID child_id = item->get_id();
    m_items.emplace_back(std::move(item));
    on_child_added(child_id);

    // update the parent layout if necessary
    if (_update_claim()) {
        _update_ancestor_layouts();
    }
    _redraw();
}

void Overlayout::remove_item(const ItemPtr& item)
{
    auto it = std::find(m_items.begin(), m_items.end(), item);
    assert(it != m_items.end());
    m_items.erase(it);
}

Aabrf Overlayout::get_content_aabr() const
{
    Aabrf result;
    for (const ItemPtr& item : m_items) {
        result.unite(get_screen_item(item.get())->get_aarbr());
    }
    return result;
}

std::unique_ptr<LayoutIterator> Overlayout::iter_items() const
{
    return std::make_unique<OverlayoutIterator>(this);
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

Claim Overlayout::_aggregate_claim()
{
    Claim new_claim;
    for (ItemPtr& item : m_items) {
        if (const ScreenItem* screen_item = get_screen_item(item.get())) {
            new_claim.maxed(screen_item->get_claim());
        }
    }
    return new_claim;
}

void Overlayout::_relayout()
{
    const Size2f total_size     = get_size();
    const Size2f available_size = {total_size.width - m_padding.width(), total_size.height - m_padding.height()};
    for (ItemPtr& item : m_items) {
        ScreenItem* screen_item = get_screen_item(item.get());
        if (!screen_item) {
            continue;
        }

        // the item's size is independent of its placement
        _set_size(screen_item, available_size);
        const Size2f item_size = screen_item->get_size();

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
        _set_layout_transform(screen_item, Xform2f::translation({x, y}));
    }
}

void Overlayout::_get_widgets_at(const Vector2f& local_pos, std::vector<Widget*>& result) const
{
    for (const ItemPtr& item : reverse(m_items)) {
        const ScreenItem* screen_item = get_screen_item(item.get());
        if (screen_item && screen_item->get_aarbr().contains(local_pos)) {
            Vector2f item_pos = local_pos;
            screen_item->get_transform().get_inverse().transform(item_pos);
            Item::_get_widgets_at(screen_item, item_pos, result);
        }
    }
}

} // namespace notf
