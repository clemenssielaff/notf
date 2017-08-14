#include "dynamic/layout/overlayout.hpp"

#include "common/log.hpp"
#include "common/vector.hpp"
#include "common/warnings.hpp"
#include "core/item_container.hpp"
#include "utils/reverse_iterator.hpp"

namespace notf {

Overlayout::Overlayout()
    : Layout(std::make_unique<detail::ItemList>())
    , m_horizontal_alignment(AlignHorizontal::CENTER)
    , m_vertical_alignment(AlignVertical::CENTER)
    , m_padding(Padding::none())
{
}

std::shared_ptr<Overlayout> Overlayout::create()
{
#ifdef _DEBUG
    return std::shared_ptr<Overlayout>(new Overlayout());
#else
    struct make_shared_enabler : public Overlayout {
        make_shared_enabler()
            : Overlayout() {}
        PADDING(4)
    };
    return std::make_shared<make_shared_enabler>();
#endif
}

void Overlayout::set_alignment(const AlignHorizontal horizontal, const AlignVertical vertical)
{
    if (horizontal == m_horizontal_alignment && vertical == m_vertical_alignment) {
        return;
    }
    m_horizontal_alignment = std::move(horizontal);
    m_vertical_alignment   = std::move(vertical);
    if (!_update_claim()) {
        _relayout();
    }
}

void Overlayout::set_padding(const Padding& padding)
{
    if (!padding.is_valid()) {
        log_critical << "Encountered invalid padding: " << padding;
        throw std::runtime_error("Encountered invalid padding");
    }
    if (padding == m_padding) {
        return;
    }
    m_padding = padding;
    if (!_update_claim()) {
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
    log_trace << "Added child Item " << item->get_name() << " to Overlayout " << get_name();

    // update the parent layout if necessary
    if (!_update_claim()) {
        // update only the child if we don't need a full Claim update
        Size2f item_grant = get_size();
        item_grant.width -= m_padding.get_width();
        item_grant.height -= m_padding.get_height();
        ScreenItem::_set_grant(item->get_screen_item(), std::move(item_grant));

        _relayout();
    }
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
        log_warning << "Cannot remove unknown child Item " << child_item->get_name()
                    << " from Overlayout " << get_name();
        return;
    }

    log_trace << "Removing child Item " << child_item->get_name() << " from Overlayout " << get_name();
    items.erase(it);
    on_child_removed(child_item);
    if (!_update_claim()) {
        _relayout();
    }
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
    result.get_horizontal().grow_by(m_padding.get_width());
    result.get_vertical().grow_by(m_padding.get_height());
    return result;
}

void Overlayout::_relayout()
{
    _set_size(get_claim().apply(get_grant())); // TODO: this line is the same in EVERY ScreenItem::_relayout! It should only update in _set_grant (and _set_claim?)
    const Size2f reference_size = get_size();
    const Size2f available_size = {reference_size.width - m_padding.get_width(),
                                   reference_size.height - m_padding.get_height()};

    // update your children's location
    detail::ItemList& children = *static_cast<detail::ItemList*>(m_children.get());
    Aabrf content_aabr         = Aabrf::zero();
    for (ItemPtr& item : children.items) {
        ScreenItem* screen_item = item->get_screen_item();
        if (!screen_item) {
            continue;
        }

        // the item's size is independent of its placement
        ScreenItem::_set_grant(screen_item, available_size);
        const Aabrf item_aabr   = screen_item->get_aabr<Space::PARENT>();
        const Size2f& item_size = item_aabr.get_size();

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
            pos.y() = (reference_size.height - item_size.height) / 2.f;
        }
        else {
            assert(m_vertical_alignment == AlignVertical::BOTTOM);
            pos.y() = m_padding.bottom;
        }

        const Xform2f item_xform = Xform2f::translation(pos);
        ScreenItem::_set_layout_xform(screen_item, item_xform);

        if (content_aabr.is_zero()) {
            content_aabr = item_xform.transform(screen_item->get_content_aabr());
        }
        else {
            content_aabr.unite(item_xform.transform(screen_item->get_content_aabr()));
        }
    }
    _set_content_aabr(std::move(content_aabr));
}

} // namespace notf
