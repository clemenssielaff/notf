#include "dynamic/layout/free_layout.hpp"

#include "common/log.hpp"
#include "common/vector.hpp"
#include "common/warnings.hpp"
#include "core/item_container.hpp"
#include "utils/reverse_iterator.hpp"

namespace notf {

FreeLayout::FreeLayout()
    : Layout(std::make_unique<detail::ItemList>())
{
}

std::shared_ptr<FreeLayout> FreeLayout::create()
{
    struct make_shared_enabler : public FreeLayout {
        make_shared_enabler()
            : FreeLayout() {}
        PADDING(7)
    };
    return std::make_shared<make_shared_enabler>();
}

void FreeLayout::_get_widgets_at(const Vector2f& local_pos, std::vector<Widget*>& result) const
{
    // just iterate over all items - this is slow but okay for now
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

void FreeLayout::add_item(ItemPtr item)
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
    _redraw();
}

void FreeLayout::_remove_child(const Item* child_item)
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
                     << " from FreeLayout " << get_id();
        return;
    }

    log_trace << "Removing child Item " << child_item->get_id() << " from FreeLayout " << get_id();
    items.erase(it);
    on_child_removed(child_item);
    _redraw();
}

Aabrf FreeLayout::get_children_aabr() const
{
    Aabrf result;
    std::vector<ItemPtr>& items = static_cast<detail::ItemList*>(m_children.get())->items;
    for (const ItemPtr& item : items) {
        result.unite(item->get_screen_item()->get_aabr(Space::PARENT));
    }
    return result;
}

Claim FreeLayout::_aggregate_claim()
{
    return {}; // TODO: FreeLayout::_aggregate_claim (?)
}

} // namespace notf
