#include "core/abstract_layout_item.hpp"

#include <assert.h>

#include "common/log.hpp"
#include "common/vector_utils.hpp"
#include "core/layout_item.hpp"
#include "core/root_layout_item.hpp"

namespace signal {

AbstractLayoutItem::~AbstractLayoutItem()
{
    assert(!has_parent());

    // Unparent the children before removal
    // If the parent holds the last shared_ptr, they are deleted anyway, otherwise (if the user holds an owning pointer)
    // at least the child will know that its parent has disappeared
    if (m_internal_child) {
        m_internal_child->unparent();
    }
    for (const std::shared_ptr<LayoutItem>& external_child : m_external_children) {
        external_child->unparent();
    }
}

std::shared_ptr<const RootLayoutItem> AbstractLayoutItem::get_root_item() const
{
    // this might be the RootWidget itself
    if (!has_parent()) {
        return std::dynamic_pointer_cast<const RootLayoutItem>(shared_from_this());
    }

    // otherwise it may be one of its ancestors
    std::shared_ptr<const AbstractLayoutItem> it = get_parent();
    while (it->has_parent()) {
        it = it->get_parent();
    }
    return std::dynamic_pointer_cast<const RootLayoutItem>(it);
}

bool AbstractLayoutItem::is_ancestor_of(const std::shared_ptr<AbstractLayoutItem> ancestor)
{
    // invalid LayoutItems can never be an ancestor
    if (!ancestor) {
        return false;
    }

    // check all actual ancestors against the candidate
    std::shared_ptr<AbstractLayoutItem> parent = get_parent();
    while (parent) {
        if (parent == ancestor) {
            return true;
        }
        parent = parent->get_parent();
    }

    // if no ancestor matches, we have our answer
    return false;
}

bool AbstractLayoutItem::set_parent(std::shared_ptr<AbstractLayoutItem> parent)
{
    // do nothing if the new parent is the same as the old (or both are invalid)
    if (parent == get_parent()) {
        return false;
    }

    // check for cyclic ancestry
    if (is_ancestor_of(parent)) {
        log_critical << "Cannot make " << parent->get_handle() << " the parent of " << get_handle()
                     << " because " << parent->get_handle() << " is already a child of " << get_handle();
        return false;
    }

    m_parent = parent;
    return true;
}

void AbstractLayoutItem::set_internal_child(std::shared_ptr<LayoutItem> child)
{
    m_internal_child = child;
    m_internal_child->set_parent(std::static_pointer_cast<AbstractLayoutItem>(shared_from_this()));
}

void AbstractLayoutItem::remove_child(std::shared_ptr<LayoutItem> child)
{
    // if the internal child is removed, just invalidate the pointer
    if (child == m_internal_child) {
        m_internal_child.reset();
    }

    // external children are properly removed
    else if (!remove_one_unordered(m_external_children, child)) {
        log_critical << "Failed to remove unknown child " << child->get_handle() << " from parent: " << get_handle();
    }
}

} // namespace signal
