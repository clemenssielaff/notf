#include "core/layout.hpp"

#include <algorithm>

#include "common/log.hpp"

namespace notf {

Layout::~Layout()
{
    // explicitly unparent all children so they can send the `parent_changed` signal
    for (std::shared_ptr<LayoutItem>& child : m_children) {
        child->_unparent();
    }
}

bool Layout::has_child(const std::shared_ptr<LayoutItem>& candidate) const
{
    for (const std::shared_ptr<LayoutItem>& child : m_children) {
        if (child == candidate) {
            return true;
        }
    }
    return false;
}

void Layout::_add_child(std::shared_ptr<LayoutItem> item)
{
    if (std::find(m_children.begin(), m_children.end(), item) != m_children.end()) {
        log_warning << "Did not add existing child " << item->get_handle() << " to LayoutItem " << get_handle();
        return;
    }
    item->_set_parent(std::static_pointer_cast<Layout>(shared_from_this()));
    const Handle child_handle = item->get_handle();
    m_children.emplace_back(std::move(item));
    child_added(child_handle);
}

void Layout::_remove_child(const LayoutItem* item)
{
    auto it = std::find(m_children.begin(), m_children.end(), item->shared_from_this());
    if (it == m_children.end()) {
        log_critical << "Failed to remove unknown child " << item->get_handle() << " from LayoutItem " << get_handle();
        return;
    }
    _remove_item(item);
    m_children.erase(it);
    child_removed(item->get_handle());
}

void Layout::_cascade_visibility(const VISIBILITY visibility)
{
    if (visibility == get_visibility()) {
        return;
    }
    LayoutItem::_cascade_visibility(visibility);

    // update your children's visiblity
    for (std::shared_ptr<LayoutItem>& child : m_children) {
        if (child->get_visibility() != VISIBILITY::INVISIBLE) {
            if (get_visibility() == VISIBILITY::INVISIBLE) {
                child->_cascade_visibility(VISIBILITY::HIDDEN);
            }
            else {
                child->_cascade_visibility(visibility);
            }
        }
    }
}

void Layout::_redraw()
{
    // redraw all children
    for (std::shared_ptr<LayoutItem>& child : m_children) {
        child->_redraw();
    }
}

} // namespace notf
