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

void Overlayout::set_padding(const Padding& padding)
{
    if (!padding.is_valid()) {
        log_critical << "Encountered invalid padding: " << padding;
        return;
    }
    if (padding != m_padding) {
        m_padding = padding;
        _update_claim();
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
    if (!_update_claim()) {
        // update only the child if we don't need a full Claim update
        Size2f item_grant = get_size();
        item_grant.width -= m_padding.width();
        item_grant.height -= m_padding.height();
        ScreenItem::_set_grant(item->get_screen_item(), std::move(item_grant));
    }
    _relayout();
}

void Overlayout::_remove_child(const Item* child_item)
{
    if (!child_item) {
        return;
    }
    std::vector<ItemPtr>& items = static_cast<detail::ItemList*>(m_children.get())->items;

    auto it = std::find_if(std::begin(items), std::end(items),
                           [child_item](const ItemPtr& item) -> bool {
                               return item.get() == child_item;
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
        if (screen_item && screen_item->get_aabr<Space::PARENT>().contains(local_pos)) {
            Vector2f item_pos = local_pos;
            screen_item->get_xform<Space::PARENT>().invert().transform(item_pos);
            ScreenItem::_get_widgets_at(screen_item, item_pos, result);
        }
    }
}

Claim Overlayout::_consolidate_claim()
{
    std::vector<ItemPtr>& items = static_cast<detail::ItemList*>(m_children.get())->items;
    if (items.empty()) {
        return Claim();
    }

    Claim result = Claim::zero();
    for (const ItemPtr& item : items) {
        if (const ScreenItem* screen_item = item->get_screen_item()) {
            result.maxed(screen_item->get_claim());
        }
    }
    result.get_horizontal().grow_by(m_padding.width());
    result.get_vertical().grow_by(m_padding.height());
    return result;
}

void Overlayout::_relayout()
{
    const Size2f reference_size = get_claim().apply(get_grant());
    const Size2f available_size = {reference_size.width - m_padding.width(),
                                   reference_size.height - m_padding.height()};

    // update your children's location
    Aabrf new_aabr = Aabrf::zero();
    for (ItemPtr& item : static_cast<detail::ItemList*>(m_children.get())->items) {
        ScreenItem* screen_item = item->get_screen_item();
        if (!screen_item) {
            continue;
        }

        // the item's size is independent of its placement
        ScreenItem::_set_grant(screen_item, available_size);
        const Size2f& item_size = screen_item->get_size();

        // the item's transform depends on the Overlayout's alignment and grant
        Vector2f pos;
        if (m_horizontal_alignment == AlignHorizontal::LEFT) {
            pos.x() = m_padding.left;
        }
        else if (m_horizontal_alignment == AlignHorizontal::CENTER) {
            pos.x() = ((available_size.width - item_size.width) / 2.f) + m_padding.left;
        }
        else {
            assert(m_horizontal_alignment == AlignHorizontal::RIGHT);
            pos.x() = reference_size.width - m_padding.right - item_size.width;
        }

        if (m_vertical_alignment == AlignVertical::TOP) {
            pos.y() = reference_size.height - m_padding.top - item_size.height;
        }
        else if (m_vertical_alignment == AlignVertical::CENTER) {
            pos.y() = ((available_size.height - item_size.height) / 2.f) + m_padding.bottom;
        }
        else {
            assert(m_vertical_alignment == AlignVertical::BOTTOM);
            pos.y() = m_padding.bottom;
        }
        _set_layout_xform(screen_item, Xform2f::translation(pos));

        if(new_aabr.is_zero()){
            new_aabr = Aabrf(pos, item_size);
        } else {
            new_aabr.unite(Aabrf(pos, item_size));
        }
    }

    // update your own aabr
    _set_aabr(std::move(new_aabr));
}

} // namespace notf
