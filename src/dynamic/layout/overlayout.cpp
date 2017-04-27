#include "dynamic/layout/overlayout.hpp"

#include <algorithm>
#include <sstream>

#include "common/aabr.hpp"
#include "common/log.hpp"
#include "common/vector.hpp"
#include "core/controller.hpp"
#include "core/screen_item.hpp"

namespace notf {

const Item* OverlayoutIterator::next()
{
    if (m_index >= m_layout->m_items.size()) {
        return nullptr;
    }
    return m_layout->m_items[m_index++].get();
}

/**********************************************************************************************************************/

struct Overlayout::make_shared_enabler : public Overlayout {
    template <typename... Args>
    make_shared_enabler(Args&&... args)
        : Overlayout(std::forward<Args>(args)...) {}
    virtual ~make_shared_enabler();
};
Overlayout::make_shared_enabler::~make_shared_enabler() {}

Overlayout::Overlayout()
    : Layout()
    , m_padding(Padding::none())
    , m_horizontal_alignment(Horizontal::CENTER)
    , m_vertical_alignment(Vertical::CENTER)
    , m_items()
{
}

std::shared_ptr<Overlayout> Overlayout::create()
{
    return std::make_shared<make_shared_enabler>();
}

Overlayout::~Overlayout()
{
    // explicitly unparent all children so they can send the `parent_changed` signal
    for (ItemPtr& item : m_items) {
        child_removed(item->get_id());
        _set_item_parent(item.get(), {});
    }
}

bool Overlayout::has_item(const ItemPtr& item) const
{
    return std::find(m_items.cbegin(), m_items.cbegin(), item) != m_items.cend();
}

void Overlayout::clear()
{
    for (ItemPtr& item : m_items) {
        child_removed(item->get_id());
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
        _set_item_size(controller->get_root_item().get(), get_size());
        // TODO: all of this should be a Layout function! ... but can it be?
    }
    else if (ScreenItem* screen_item = dynamic_cast<ScreenItem*>(item.get())) {
        _set_item_size(screen_item, get_size());
    }

    // take ownership of the Item
    _set_item_parent(item.get(), shared_from_this());
    const ItemID child_id = item->get_id();
    m_items.emplace_back(std::move(item));
    child_added(child_id);

    // update the parent layout if necessary
    if (_update_claim()) {
        _update_parent_layout();
    }
    _redraw();
}

void Overlayout::remove_item(const ItemPtr& item)
{
    auto it = std::find(m_items.begin(), m_items.end(), item);
    assert(it != m_items.end());
    m_items.erase(it);
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
        _set_item_size(screen_item, available_size);
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
        screen_item->set_transform(Xform2f::translation({x, y}));
    }
}

void Overlayout::_get_widgets_at(const Vector2f& local_pos, std::vector<Widget*>& result) const
{
    // TODO: Overlayout::get_widget_at does not respect transform (only translate)
    for (const ItemPtr& item : m_items) {
        const ScreenItem* screen_item = get_screen_item(item.get());
        if (!screen_item) {
            continue;
        }
        const Vector2f item_pos = local_pos - screen_item->get_transform().get_translation();
        const Aabrf item_rect(screen_item->get_size());
        if (item_rect.contains(item_pos)) {
            _get_widgets_at_item_pos(screen_item, item_pos, result);
        }
    }
    // TODO: Overlayout::_get_widgets_at iterates the wrong way around (from back to front)
}

} // namespace notf
