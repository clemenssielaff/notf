#include "core/layout_item.hpp"

#include "common/log.hpp"
#include "core/application.hpp"
#include "core/item_manager.hpp"
#include "core/window.hpp"

namespace signal {

LayoutItem::~LayoutItem()
{
    // send the signal before actually changing anything
    about_to_be_destroyed();

    // unparent this LayoutItem and let all children update their visibility to unrooted
    unparent();

    // unparent the children, they are already unrooted so their visibility won't update again
    if (m_internal_child) {
        m_internal_child->set_parent({});
    }
    for (const std::shared_ptr<LayoutItem>& external_child : m_external_children) {
        external_child->unparent();
    }
}

void LayoutItem::set_visible(const bool is_visible)
{
    // ignore non-changes
    if ((is_visible && m_visibility == VISIBILITY::VISIBLE) || (!is_visible && m_visibility == VISIBILITY::INVISIBLE)) {
        return;
    }
    const VISIBILITY previous_visibility = m_visibility;

    // update visibility
    if (is_visible) {
        std::shared_ptr<AbstractLayoutItem> abstract_parent = get_parent();
        if (!abstract_parent) {
            update_visibility(VISIBILITY::UNROOTED);
        }
        else {
            std::shared_ptr<LayoutItem> parent = std::dynamic_pointer_cast<LayoutItem>(abstract_parent);
            if (parent) {
                const VISIBILITY parent_visibility = parent->get_visibility();
                if (parent_visibility == VISIBILITY::INVISIBLE) {
                    update_visibility(VISIBILITY::HIDDEN);
                }
                else {
                    update_visibility(parent_visibility);
                }
            }
            else {
                update_visibility(VISIBILITY::VISIBLE);
            }
        }
    }
    else {
        update_visibility(VISIBILITY::INVISIBLE);
    }

    // do nothing if the update didn't change anything
    if (previous_visibility == m_visibility) {
        return;
    }

    // only redraw if the LayoutItem either appeared or disappeared
    if ((previous_visibility == VISIBILITY::VISIBLE) || (m_visibility == VISIBILITY::VISIBLE)) {
        redraw();
    }
}

bool LayoutItem::set_parent(std::shared_ptr<AbstractLayoutItem> parent)
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

    // update visibility
    if (!parent) {
        if (m_visibility != VISIBILITY::INVISIBLE) {
            update_visibility(VISIBILITY::UNROOTED);
        }
    }
    else {
        VISIBILITY parent_visibility;
        if (std::shared_ptr<LayoutItem> parent_layout_item = std::dynamic_pointer_cast<LayoutItem>(parent)) {
            parent_visibility = parent_layout_item->get_visibility();
        }
        else {
            parent_visibility = VISIBILITY::VISIBLE;
        }
        if (parent_visibility == VISIBILITY::INVISIBLE) {
            if (m_visibility != VISIBILITY::INVISIBLE) {
                update_visibility(VISIBILITY::HIDDEN);
            }
        }
        else if (m_visibility != VISIBILITY::INVISIBLE) {
            update_visibility(parent_visibility);
        }
    }
    return true;
}

void LayoutItem::get_window_transform_impl(Transform2& result) const
{
    if (std::shared_ptr<LayoutItem> parent = std::dynamic_pointer_cast<LayoutItem>(get_parent())) {
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
    if (visibility == m_visibility) {
        return;
    }

    // update your own visibility
    m_visibility = visibility;
    visibility_changed(m_visibility);

    // update your children's visiblity
    if (m_internal_child) {
        if (m_internal_child->m_visibility != VISIBILITY::INVISIBLE) {
            if (m_visibility == VISIBILITY::INVISIBLE) {
                m_internal_child->update_visibility(VISIBILITY::HIDDEN);
            }
            else {
                m_internal_child->update_visibility(m_visibility);
            }
        }
    }
    for (const std::shared_ptr<LayoutItem>& external_child : m_external_children) {
        if (external_child->m_visibility != VISIBILITY::INVISIBLE) {
            if (m_visibility == VISIBILITY::INVISIBLE) {
                external_child->update_visibility(VISIBILITY::HIDDEN);
            }
            else {
                external_child->update_visibility(m_visibility);
            }
        }
    }
}

} // namespace signal
