#include "dynamic/layout/free_layout.hpp"

#include "common/aabr.hpp"
#include "common/log.hpp"
#include "common/vector.hpp"
#include "core/controller.hpp"
#include "core/screen_item.hpp"
#include "utils/reverse_iterator.hpp"

namespace notf {

const Item* FreeLayoutIterator::next()
{
    if (m_index >= m_layout->m_items.size()) {
        return nullptr;
    }
    return m_layout->m_items[m_index++].get();
}

/**********************************************************************************************************************/

struct FreeLayout::make_shared_enabler : public FreeLayout {
    template <typename... Args>
    make_shared_enabler(Args&&... args)
        : FreeLayout(std::forward<Args>(args)...) {}
    virtual ~make_shared_enabler();
};
FreeLayout::make_shared_enabler::~make_shared_enabler() {}

FreeLayout::FreeLayout()
    : Layout()
    , m_items()
{
}

FreeLayoutPtr FreeLayout::create()
{
    return std::make_shared<make_shared_enabler>();
}

FreeLayout::~FreeLayout()
{
    // explicitly unparent all children so they can send the `parent_changed` signal
    for (ItemPtr& item : m_items) {
        child_removed(item->get_id());
        _set_item_parent(item.get(), {});
    }
}

bool FreeLayout::has_item(const ItemPtr& item) const
{
    return std::find(m_items.cbegin(), m_items.cbegin(), item) != m_items.cend();
}

void FreeLayout::clear()
{
    for (ItemPtr& item : m_items) {
        child_removed(item->get_id());
    }
    m_items.clear();
}

void FreeLayout::add_item(ItemPtr item)
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

void FreeLayout::remove_item(const ItemPtr& item)
{
    auto it = std::find(m_items.begin(), m_items.end(), item);
    assert(it != m_items.end());
    m_items.erase(it);
}

std::unique_ptr<LayoutIterator> FreeLayout::iter_items() const
{
    return std::make_unique<FreeLayoutIterator>(this);
}

void FreeLayout::_get_widgets_at(const Vector2f& local_pos, std::vector<Widget*>& result) const
{
    // TODO: FreeLayout::get_widget_at does not respect transform (only translate) and is horribly inefficient
    for (const ItemPtr& item : reverse(m_items)) {
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
}

} // namespace notf
