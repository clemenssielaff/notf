#include "dynamic/layout/text_layout.hpp"

#include "common/log.hpp"
#include "common/vector.hpp"
#include "common/warnings.hpp"
#include "core/item_container.hpp"
#include "core/widget.hpp"
#include "dynamic/widget/capability/text_capability.hpp"
#include "utils/reverse_iterator.hpp"

namespace notf {

TextLayout::TextLayout()
    : Layout(std::make_unique<detail::ItemList>())
    , m_padding(Padding::none())
{
}

std::shared_ptr<TextLayout> TextLayout::create()
{
#ifdef _DEBUG
    return std::shared_ptr<TextLayout>(new TextLayout());
#else
    struct make_shared_enabler : public TextLayout {
        make_shared_enabler()
            : TextLayout() {}
        PADDING(4)
    };
    return std::make_shared<make_shared_enabler>();
#endif
}

void TextLayout::set_padding(const Padding& padding)
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

void TextLayout::add_item(ItemPtr item)
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
    log_trace << "Added child Item " << item->get_name() << " to TextLayout " << get_name();

    if (!_update_claim()) {
        _relayout();
    }
}

void TextLayout::_remove_child(const Item* child_item)
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

    log_trace << "Removing child Item " << child_item->get_name() << " from TextLayout " << get_name();
    items.erase(it);
    on_child_removed(child_item);

    if (!_update_claim()) {
        _relayout();
    }
}

void TextLayout::_get_widgets_at(const Vector2f& local_pos, std::vector<Widget*>& result) const
{
    for (const ItemPtr& item : static_cast<detail::ItemList*>(m_children.get())->items) {
        const ScreenItem* screen_item = item->get_screen_item();
        if (screen_item && screen_item->get_aabr<Space::PARENT>().contains(local_pos)) {
            Vector2f item_pos = local_pos;
            screen_item->get_xform<Space::PARENT>().invert().transform(item_pos);
            ScreenItem::_get_widgets_at(screen_item, item_pos, result);
        }
    }
}

Claim TextLayout::_consolidate_claim()
{
    std::vector<ItemPtr>& items = static_cast<detail::ItemList*>(m_children.get())->items;
    if (items.empty()) {
        return Claim();
    }

    Claim result = Claim::zero();
    for (const ItemPtr& item : items) {
        if (const ScreenItem* screen_item = item->get_screen_item()) {
            result.add_vertical(screen_item->get_claim());
        }
    }
    result.get_horizontal().grow_by(m_padding.get_width());
    result.get_vertical().grow_by(m_padding.get_height());
    return result;
}

void TextLayout::_relayout()
{
    _set_size(get_claim().apply(get_grant()));
    const float available_width = get_size().width - m_padding.get_width();
    detail::ItemList& children  = *static_cast<detail::ItemList*>(m_children.get());

    Vector2f stylus(0, get_size().height);
    bool stylus_on_baseline = false;
    for (ItemPtr& item : children.items) {
        ScreenItem* screen_item = item->get_screen_item();
        if (!screen_item) {
            continue;
        }

        Widget* widget = dynamic_cast<Widget*>(screen_item);
        TextCapabilityPtr text_capability;

        // define the start of the widget's baseline before setting its size
        if (widget) {
            text_capability = widget->capability<TextCapability>();
            if (text_capability) {
                if (stylus_on_baseline) {
                    text_capability->baseline_start = stylus;
                }
                else {
                    text_capability->baseline_start = Vector2f(0, stylus.y() - text_capability->font->ascender());
                }
            }
        }

        // force the minimal vertical size for each child
        ScreenItem::_set_grant(screen_item, Size2f(available_width, 0));

        // advance the stylus
        if (text_capability) {
            stylus             = text_capability->baseline_end;
            stylus_on_baseline = true;
        } else {
            stylus             = Vector2f(0, stylus.y() - screen_item->get_size().height);
            stylus_on_baseline = false;
        }
    }

    // update the children's location
    float offset       = get_size().height;
    Aabrf content_aabr = Aabrf::zero();
    for (ItemPtr& item : children.items) {
        ScreenItem* screen_item = item->get_screen_item();
        if (!screen_item) {
            continue;
        }

        offset -= screen_item->get_size().height;
        const Xform2f item_xform = Xform2f::translation(m_padding.left, offset);
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
