#include "core/layout_object.hpp"

#include "common/log.hpp"
#include "core/layout.hpp"
#include "core/layout_root.hpp"

namespace signal {

LayoutItem::~LayoutItem()
{
}

LayoutObject::~LayoutObject()
{
    // explicitly unparent all children so they can send the `parent_changed` signal
    for (auto& it : m_children) {
        it.second->get_object()->unparent();
    }
}

bool LayoutObject::has_child(const std::shared_ptr<LayoutObject>& candidate) const
{
    for (const auto& it : m_children) {
        if (it.second->get_object() == candidate) {
            return true;
        }
    }
    return false;
}

bool LayoutObject::has_ancestor(const std::shared_ptr<LayoutObject>& ancestor) const
{
    // invalid LayoutObject can never be an ancestor
    if (!ancestor) {
        return false;
    }

    // check all actual ancestors against the candidate
    std::shared_ptr<const LayoutObject> parent = get_parent();
    while (parent) {
        if (parent == ancestor) {
            return true;
        }
        parent = parent->get_parent();
    }

    // if no ancestor matches, we have our answer
    return false;
}

std::shared_ptr<const LayoutRoot> LayoutObject::get_root() const
{
    std::shared_ptr<const LayoutObject> it = std::static_pointer_cast<const LayoutObject>(shared_from_this());
    while (it->has_parent()) {
        it = it->get_parent();
    }
    return std::dynamic_pointer_cast<const LayoutRoot>(it);
}

const std::unique_ptr<LayoutItem>& LayoutObject::get_child(const Handle child_handle) const
{
    static const std::unique_ptr<LayoutItem> INVALID;
    auto it = m_children.find(child_handle);
    if (it == m_children.end()) {
        log_warning << "Requested unknown child" << child_handle << " from LayoutObject " << get_handle();
        return INVALID;
    }
    return it->second;
}

void LayoutObject::add_child(std::unique_ptr<LayoutItem> child_item)
{
    const std::shared_ptr<LayoutObject>& child_object = child_item->get_object();
    const Handle child_handle = child_object->get_handle();
    if (m_children.count(child_handle)) {
        log_warning << "Did not add existing child " << child_handle << " to LayoutObject " << get_handle();
        return;
    }
    child_object->set_parent(std::static_pointer_cast<LayoutObject>(shared_from_this()));
    m_children.emplace(std::make_pair(child_handle, std::move(child_item)));
    child_added(child_handle);
}

void LayoutObject::remove_child(const Handle child_handle)
{
    auto it = m_children.find(child_handle);
    if (it == m_children.end()) {
        log_critical << "Failed to remove unknown child " << child_handle << " from LayoutObject " << get_handle();
    }
    else {
        m_children.erase(it);
        child_removed(child_handle);
    }
}

void LayoutObject::set_visible(const bool is_visible)
{
    // ignore non-changes
    if ((is_visible && m_visibility == VISIBILITY::VISIBLE) || (!is_visible && m_visibility == VISIBILITY::INVISIBLE)) {
        return;
    }

    // update visibility
    const VISIBILITY previous_visibility = m_visibility;
    if (is_visible) {
        if (std::shared_ptr<const LayoutObject> parent = get_parent()) {
            const VISIBILITY parent_visibility = parent->get_visibility();
            if (parent_visibility == VISIBILITY::INVISIBLE) {
                cascade_visibility(VISIBILITY::HIDDEN);
            }
            else {
                cascade_visibility(parent_visibility);
            }
        }
        else {
            cascade_visibility(VISIBILITY::UNROOTED);
        }
    }
    else {
        cascade_visibility(VISIBILITY::INVISIBLE);
    }

    // redraw if the object just became visible
    if (previous_visibility != m_visibility && m_visibility == VISIBILITY::VISIBLE) {
        redraw();
    }
}

void LayoutObject::redraw()
{
    // don't draw invisible objects
    if (m_visibility != VISIBILITY::VISIBLE) {
        return;
    }

    // redraw all children
    for (auto& it : m_children) {
        it.second->get_object()->redraw();
    }
}

void LayoutObject::set_parent(std::shared_ptr<LayoutObject> parent)
{
    // do nothing if the new parent is the same as the old (or both are invalid)
    std::shared_ptr<LayoutObject> old_parent = m_parent.lock();
    if (parent == old_parent) {
        return;
    }

    // check for cyclic ancestry
    if (has_ancestor(parent)) {
        log_critical << "Cannot make " << parent->get_handle() << " the parent of " << get_handle()
                     << " because " << parent->get_handle() << " is already a child of " << get_handle();
        return;
    }

    // update your parent
    if (old_parent) {
        old_parent->remove_child(get_handle());
    }

    m_parent = parent;
    parent_changed(parent->get_handle());

    // update visibility
    if (!parent) {
        if (m_visibility != VISIBILITY::INVISIBLE) {
            cascade_visibility(VISIBILITY::UNROOTED);
        }
    }
    else {
        VISIBILITY parent_visibility = parent->get_visibility();
        if (parent_visibility == VISIBILITY::INVISIBLE) {
            if (m_visibility != VISIBILITY::INVISIBLE) {
                cascade_visibility(VISIBILITY::HIDDEN);
            }
        }
        else if (m_visibility != VISIBILITY::INVISIBLE) {
            cascade_visibility(parent_visibility);
        }
    }
}

void LayoutObject::cascade_visibility(const VISIBILITY visibility)
{
    // update your own visibility
    if (visibility == m_visibility) {
        return;
    }
    m_visibility = visibility;
    visibility_changed(m_visibility);

    // update your children's visiblity
    for (auto& it : m_children) {
        const std::shared_ptr<LayoutObject>& child = it.second->get_object();
        if (child->get_visibility() != VISIBILITY::INVISIBLE) {
            if (m_visibility == VISIBILITY::INVISIBLE) {
                child->cascade_visibility(VISIBILITY::HIDDEN);
            }
            else {
                child->cascade_visibility(visibility);
            }
        }
    }
}

void LayoutObject::get_window_transform(Transform2& result) const
{
    if (std::shared_ptr<const LayoutObject> parent = get_parent()) {
        parent->get_window_transform(result);
    }
    result = m_transform * result;
}

Transform2 LayoutObject::get_screen_transform() const
{
    log_critical << "get_transform(SPACE::SCREEN) is not emplemented yet";
    return get_parent_transform();
}

} // namespace signal
