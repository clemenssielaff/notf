#include "core/layout.hpp"

#include "common/log.hpp"
#include "core/controller.hpp"

namespace notf {

Layout::~Layout()
{
    // explicitly unparent all children so they can send the `parent_changed` signal
    for (std::shared_ptr<Item>& child : m_children) {
        _set_item_parent(child.get(), {});
    }
}

bool Layout::has_item(const std::shared_ptr<Item>& candidate) const
{
    for (const std::shared_ptr<Item>& child : m_children) {
        if (child == candidate) {
            return true;
        }
    }
    return false;
}

void Layout::_add_child(std::shared_ptr<Item> item)
{
    if (std::find(m_children.begin(), m_children.end(), item) != m_children.end()) {
        log_warning << "Did not add existing child " << item->get_id() << " to Item " << get_id();
        return;
    }

    // Controllers are initialized the first time they are parented to a Layout
    if(Controller* controller = dynamic_cast<Controller*>(item.get())){
        controller->initialize();
    }

    // take ownership of the Item
    _set_item_parent(item.get(), std::static_pointer_cast<Layout>(shared_from_this()));
    const ItemID child_id = item->get_id();
    m_children.emplace_back(std::move(item));
    child_added(child_id);
}

void Layout::_remove_child(const Item* item)
{
    auto it = std::find(m_children.begin(), m_children.end(), item->shared_from_this());
    if (it == m_children.end()) {
        log_critical << "Failed to remove unknown child " << item->get_id() << " from Item " << get_id();
        return;
    }
    _remove_item(item);
    m_children.erase(it);
    child_removed(item->get_id());
}

bool Layout::_set_size(const Size2f& size)
{
    if (ScreenItem::_set_size(size)) {
        _relayout();
        return true;
    }
    return false;
}

} // namespace notf
