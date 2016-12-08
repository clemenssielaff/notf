#include "core/layout.hpp"

#include <algorithm>

#include "common/log.hpp"

namespace notf {

Layout::~Layout()
{
    // explicitly unparent all children so they can send the `parent_changed` signal
    for (std::shared_ptr<Item>& child : m_children) {
        child->_unparent();
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
        log_warning << "Did not add existing child " << item->get_id() << " to LayoutItem " << get_id();
        return;
    }
    item->_set_parent(std::static_pointer_cast<Layout>(shared_from_this()));
    const ItemID child_id = item->get_id();
    m_children.emplace_back(std::move(item));
    child_added(child_id);
}

void Layout::_remove_child(const Item* item)
{
    auto it = std::find(m_children.begin(), m_children.end(), item->shared_from_this());
    if (it == m_children.end()) {
        log_critical << "Failed to remove unknown child " << item->get_id() << " from LayoutItem " << get_id();
        return;
    }
    _remove_item(item);
    m_children.erase(it);
    child_removed(item->get_id());
}

bool Layout::_set_size(const Size2f size)
{
    if(Item::_set_size(size)){
        _relayout();
        return true;
    }
    return false;
}

} // namespace notf
