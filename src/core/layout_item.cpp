#include "core/layout_item.hpp"

#include "common/log.hpp"
#include "common/vector_utils.hpp"
#include "core/application.hpp"
#include "core/layout_item_manager.hpp"
#include "core/window.hpp"

namespace signal {

LayoutItem::~LayoutItem()
{
    // send the signal before actually changing anything
    about_to_be_destroyed();

    // unparent this LayoutItem and let all children update their visibility to unrooted
    unparent();

    // release the manager's weak_ptr
    Application::get_instance().get_layout_item_manager().release_item(m_handle);

    // unparent the children, they are already unrooted so their visibility won't update again
    if (m_internal_child) {
        m_internal_child->unparent();
    }
    for (std::shared_ptr<LayoutItem>& external_child : m_external_children) {
        external_child->unparent();
    }

    log_trace << "Destroyed LayoutItem with handle:" << get_handle();
}

bool LayoutItem::is_ancestor_of(const std::shared_ptr<LayoutItem> ancestor)
{
    // invalid LayoutItems can never be an ancestor
    if (!ancestor) {
        return false;
    }

    // check all actual ancestors against the candidate
    std::shared_ptr<LayoutItem> parent = get_parent();
    while (parent) {
        if (parent == ancestor) {
            return true;
        }
        parent = parent->get_parent();
    }

    // if no ancestor matches, we have our answer
    return false;
}

std::shared_ptr<const WindowWidget> LayoutItem::get_window_widget() const
{
    // this might be the WindowWidget itself
    if (m_parent.expired()) {
        return std::dynamic_pointer_cast<const WindowWidget>(shared_from_this());
    }

    // otherwise it may be one of its ancestors
    std::shared_ptr<const LayoutItem> it = get_parent();
    while (!it->m_parent.expired()) {
        it = it->get_parent();
    }
    return std::dynamic_pointer_cast<const WindowWidget>(it);
}

void LayoutItem::set_visible(const bool is_visible)
{
    // ignore non-changes
    if ((is_visible && m_visibility == VISIBILITY::VISIBLE) || (!is_visible && m_visibility == VISIBILITY::INVISIBLE)) {
        return;
    }
    const VISIBILITY previous_visibility = m_visibility;

    // update visibility
    if(is_visible){
        std::shared_ptr<LayoutItem> parent = get_parent();
        if(parent){
            if(parent->m_visibility == VISIBILITY::INVISIBLE){
                update_visibility(VISIBILITY::HIDDEN);
            } else {
                update_visibility(parent->m_visibility);
            }
        } else {
            update_visibility(VISIBILITY::UNROOTED);
        }
    }
    else {
        if(dynamic_cast<const WindowWidget*>(this)){
            return; // WindowWidgets cannot be hidden
        }
        update_visibility(VISIBILITY::INVISIBLE);
    }

    // do nothing if the update didn't change anything
    if(previous_visibility == m_visibility){
        return;
    }

    // only redraw if the LayoutItem either appeared or disappeared
    if ((previous_visibility == VISIBILITY::VISIBLE) || (m_visibility == VISIBILITY::VISIBLE)) {
        redraw();
    }
}

void LayoutItem::set_parent(std::shared_ptr<LayoutItem> parent)
{
    // do nothing if the new parent is the same as the old (or both are invalid)
    if (parent == m_parent.lock()) {
        return;
    }

    // check for cyclic ancestry
    if (is_ancestor_of(parent)) {
        log_critical << "Cannot make " << parent->get_handle() << " the parent of " << get_handle()
                     << " because " << parent->get_handle() << " is already a child of " << get_handle();
        return;
    }
    m_parent = parent;

    // update visibility
    if(!parent){
        if (m_visibility != VISIBILITY::INVISIBLE) {
            update_visibility(VISIBILITY::UNROOTED);
        }
    }
    else if (parent->m_visibility == VISIBILITY::INVISIBLE) {
        if (m_visibility != VISIBILITY::INVISIBLE) {
            update_visibility(VISIBILITY::HIDDEN);
        }
    }
    else if (m_visibility != VISIBILITY::INVISIBLE) {
        update_visibility(parent->m_visibility);
    }
}

void LayoutItem::remove_child(std::shared_ptr<LayoutItem> child)
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

void LayoutItem::get_window_transform_impl(Transform2& result) const
{
    if (std::shared_ptr<LayoutItem> parent = m_parent.lock()) {
        parent->get_window_transform_impl(result);
    }
    result = m_transform * result;
}

Transform2 LayoutItem::get_screen_transform() const
{
    log_critical << "get_transform(SPACE::SCREEN) is not emplemented yet";
    return get_parent_transform();
}

void LayoutItem::update_visibility(const VISIBILITY visibility)
{
    // ignore non-changes
    if(visibility == m_visibility){
        return;
    }

    // update your own visibility
    m_visibility = visibility;
    visibility_changed(m_visibility);

    // update your children's visiblity
    if (m_internal_child) {
        if (m_internal_child->m_visibility != VISIBILITY::INVISIBLE) {
            if(m_visibility == VISIBILITY::INVISIBLE){
                m_internal_child->update_visibility(VISIBILITY::HIDDEN);
            } else {
                m_internal_child->update_visibility(m_visibility);
            }
        }
    }
    for (const std::shared_ptr<LayoutItem>& external_child : m_external_children) {
        if (external_child->m_visibility != VISIBILITY::INVISIBLE) {
            if(m_visibility == VISIBILITY::INVISIBLE){
                external_child->update_visibility(VISIBILITY::HIDDEN);
            } else {
                external_child->update_visibility(m_visibility);
            }
        }
    }
}

} // namespace signal
