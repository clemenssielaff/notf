#include "dynamic/layout/overlayout.hpp"

#include "common/log.hpp"
#include "common/vector.hpp"
#include "common/warnings.hpp"
#include "core/item_container.hpp"
#include "utils/reverse_iterator.hpp"

namespace notf {

Overlayout::Overlayout()
    : Layout(std::make_unique<detail::ItemList>())
    , m_horizontal_alignment(AlignHorizontal::LEFT)
    , m_vertical_alignment(AlignVertical::TOP)
    , m_padding(Padding::none())
{
}

std::shared_ptr<Overlayout> Overlayout::create()
{
    struct make_shared_enabler : public Overlayout {
        make_shared_enabler()
            : Overlayout() {}
        PADDING(4)
    };
    return std::make_shared<make_shared_enabler>();
}

Aabrf Overlayout::get_children_aabr() const
{
    Aabrf result                = Aabrf::wrongest();
    std::vector<ItemPtr>& items = static_cast<detail::ItemList*>(m_children.get())->items;
    for (const ItemPtr& item : items) {
        result.unite(item->get_screen_item()->get_aabr(Space::PARENT));
    }
    return result;
}

void Overlayout::set_padding(const Padding& padding)
{
    if (!padding.is_valid()) {
        log_critical << "Encountered invalid padding: " << padding;
        return;
    }
    if (padding != m_padding) {
        m_padding = padding;
        _relayout();
    }
}

void Overlayout::add_item(ItemPtr item)
{
    if (!item) {
        log_warning << "Cannot add an empty Item pointer to a Layout";
        return;
    }

    // if the item is already child of this Layout, place it at the end
    std::vector<ItemPtr>& items = static_cast<detail::ItemList*>(m_children.get())->items;
    if (has_child(item.get())) {
        remove_one_unordered(items, item);
    }

    // take ownership of the Item
    items.emplace_back(item);
    Item::_set_parent(item.get(), this);
    on_child_added(item.get());

    // update the parent layout if necessary
    if (_update_claim()) {
        _update_ancestor_layouts();
    }
    else { // otherwise just update the child
        ScreenItem::_set_size(item->get_screen_item(), _get_size());
    }
    _redraw();
}

void Overlayout::_remove_child(const Item* child_item)
{
    if (!child_item) {
        return;
    }

    std::vector<ItemPtr>& items = static_cast<detail::ItemList*>(m_children.get())->items;

    auto it = std::find_if(std::begin(items), std::end(items),
                           [child_item](const ItemPtr& it) -> bool {
                               return it.get() == child_item;
                           });

    if (it == std::end(items)) {
        log_critical << "Cannot remove unknown child Item " << child_item->get_id()
                     << " from Overlayout " << get_id();
        return;
    }

    log_trace << "Removing child Item " << child_item->get_id() << " from Overlayout " << get_id();
    items.erase(it);
    on_child_removed(child_item);
    _redraw();
}

void Overlayout::_get_widgets_at(const Vector2f& local_pos, std::vector<Widget*>& result) const
{
    std::vector<ItemPtr>& items = static_cast<detail::ItemList*>(m_children.get())->items;
    for (const ItemPtr& item : reverse(items)) {
        const ScreenItem* screen_item = item->get_screen_item();
        if (screen_item && screen_item->get_aabr(Space::PARENT).contains(local_pos)) {
            Vector2f item_pos = local_pos;
            screen_item->get_transform().get_inverse().transform(item_pos);
            ScreenItem::_get_widgets_at(screen_item, item_pos, result);
        }
    }
}

Claim Overlayout::_aggregate_claim()
{
    Claim result;
    std::vector<ItemPtr>& items = static_cast<detail::ItemList*>(m_children.get())->items;
    for (const ItemPtr& item : items) {
        result.maxed(item->get_screen_item()->get_claim());
    }
    return result;
}

void Overlayout::_relayout()
{
    const Size2f& total_size    = _get_size();
    const Size2f available_size = {total_size.width - m_padding.width(), total_size.height - m_padding.height()};
    std::vector<ItemPtr>& items = static_cast<detail::ItemList*>(m_children.get())->items;
    for (ItemPtr& item : items) {
        ScreenItem* screen_item = item->get_screen_item();
        if (!screen_item) {
            continue;
        }

        // the item's size is independent of its placement
        ScreenItem::_set_size(screen_item, available_size);
        const Size2f item_size = screen_item->get_aabr(Space::PARENT).get_size();

        // the item's transform depends on the Overlayout's alignment
        float x;
        if (m_horizontal_alignment == AlignHorizontal::LEFT) {
            x = m_padding.left;
        }
        else if (m_horizontal_alignment == AlignHorizontal::CENTER) {
            x = ((available_size.width - item_size.width) / 2.f) + m_padding.left;
        }
        else {
            assert(m_horizontal_alignment == AlignHorizontal::RIGHT);
            x = total_size.width - m_padding.right - item_size.width;
        }

        float y;
        if (m_vertical_alignment == AlignVertical::TOP) {
            y = m_padding.top;
        }
        else if (m_vertical_alignment == AlignVertical::CENTER) {
            y = ((available_size.height - item_size.height) / 2.f) + m_padding.top;
        }
        else {
            assert(m_vertical_alignment == AlignVertical::BOTTOM);
            y = total_size.height - m_padding.bottom - item_size.height;
        }
        _set_layout_transform(screen_item, Xform2f::translation({x, y}));
    }
}

} // namespace notf
