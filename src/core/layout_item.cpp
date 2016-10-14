#include "core/layout_item.hpp"

#include "assert.h"

#include "common/log.hpp"
#include "common/vector_utils.hpp"
#include "core/application.hpp"
#include "core/layout_item_manager.hpp"
#include "core/window.hpp"

namespace signal {

LayoutItem::~LayoutItem()
{
    visibility_changed(VISIBILITY::UNROOTED);
    about_to_be_destroyed();

    auto& manager = Application::get_instance().get_layout_item_manager();

    // set your children's parent to BAD_HANDLE - this way, even if the user kept ownership of the child, it will know
    // that this LayoutItem is gone
    if (m_internal_child) {
        m_internal_child->set_parent({});
    }
    for (std::shared_ptr<LayoutItem>& external_child : m_external_children) {
        external_child->set_parent({}); // TODO: propagate state change downstream
    }

    // release the manager's weak_ptr
    manager.release_item(m_handle);

    log_trace << "Destroyed LayoutItem with handle:" << get_handle();
}

bool LayoutItem::is_ancestor_of(const std::shared_ptr<LayoutItem> ancestor)
{
    std::shared_ptr<LayoutItem> parent = get_parent();
    while (parent) {
        if (parent == ancestor) {
            return true;
        }
        parent = parent->get_parent();
    }
    return false;
}

const std::shared_ptr<WindowWidget> LayoutItem::get_window_widget() const
{
    // this might be the WindowWidget itself
    if (m_parent.expired()) {
        return std::dynamic_pointer_cast<WindowWidget>(shared_from_this());
    }

    // otherwise it may be one of its ancestors
    std::shared_ptr<const LayoutItem> it = get_parent();
    while (!it->m_parent.expired()) {
        it = it->get_parent();
    }
    return std::dynamic_pointer_cast<WindowWidget>(it);
}

LayoutItem::VISIBILITY LayoutItem::get_visibility() const
{
    if (!m_is_visible) {
        return VISIBILITY::INVISIBLE;
    }

    std::shared_ptr<const LayoutItem> current = shared_from_this();
    std::shared_ptr<const LayoutItem> next_parent = get_parent();
    while (next_parent) {
        if (!next_parent->m_is_visible) {
            return VISIBILITY::HIDDEN;
        }
        current = next_parent;
        next_parent = current->get_parent();
    }

    if (std::dynamic_pointer_cast<WindowWidget>(current)) { // TODO: WindowWidgets may never be hidden
        return VISIBILITY::VISIBLE;
    }
    else {
        return VISIBILITY::UNROOTED;
    }
}

void LayoutItem::set_visible(const bool is_visible)
{
    // ignore non-changes
    if (is_visible == m_is_visible) {
        return;
    }

    // detect how the change affects visiblity
    const VISIBILITY previous_visiblity = get_visibility();
    m_is_visible = is_visible;
    const VISIBILITY new_visiblity = get_visibility();

    // it is possible that the change didn't affect visiblity at all (for example of a hidden LayoutItem)
    if (new_visiblity == previous_visiblity) {
        return;
    }
    visibility_changed(new_visiblity);

    // if the LayoutItem just changed visibility, all children must emit their corresponding signals
    if (new_visiblity == VISIBILITY::VISIBLE) {
        if (m_internal_child) {
            m_internal_child->emit_visibility_change_downstream(true);
        }
        for (std::shared_ptr<LayoutItem>& external_child : m_external_children) {
            external_child->emit_visibility_change_downstream(true);
        }
    }
    else if (previous_visiblity == VISIBILITY::VISIBLE) {
        if (m_internal_child) {
            m_internal_child->emit_visibility_change_downstream(false);
        }
        for (std::shared_ptr<LayoutItem>& external_child : m_external_children) {
            external_child->emit_visibility_change_downstream(false);
        }
    }

    // if the LayoutItem has neither appeared or disappeared, don't redraw
    else {
        return;
    }

    redraw();
}

void LayoutItem::set_parent(std::shared_ptr<LayoutItem> parent)
{
    if (parent == m_parent.lock()) {
        return;
    }

    const VISIBILITY previous_visiblity = get_visibility();

    if (!parent) {
        if (previous_visiblity != VISIBILITY::UNROOTED) {
            visibility_changed(VISIBILITY::UNROOTED);
        }
        return;
    }

    // check for cyclic ancestry
    if (is_ancestor_of(parent)) {
        log_critical << "Cannot make " << parent->get_handle() << " the parent of "
                     << m_handle << " because " << parent->get_handle() << " is already a child of " << get_handle();
        return;
    }
    m_parent = parent;

    // reparenting may show or hide this LayoutItem
    const VISIBILITY new_visiblity = get_visibility();
    if (previous_visiblity != new_visiblity) {
        visibility_changed(new_visiblity);
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

void LayoutItem::emit_visibility_change_downstream(bool is_parent_visible) const
{
    // invisible children stay invisible
    if (!m_is_visible) {
        return;
    }

    // visible children either become visible or hidden
    if (is_parent_visible) {
        visibility_changed(VISIBILITY::VISIBLE);
    }
    else {
        visibility_changed(VISIBILITY::HIDDEN);
    }

    // propagate further downstream
    if (m_internal_child) {
        m_internal_child->emit_visibility_change_downstream(is_parent_visible);
    }
    for (const std::shared_ptr<LayoutItem>& external_child : m_external_children) {
        external_child->emit_visibility_change_downstream(is_parent_visible);
    }
}

} // namespace signal

// TODO: visibility is overloaded as it is, maybe there should be a state_changed and a separate visibility_changed
