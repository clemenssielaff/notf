#include "core/layout.hpp"

#include "common/log.hpp"

namespace signal {

LayoutObject::ChildIteratorBase::~ChildIteratorBase()
{
}

LayoutObject::~LayoutObject()
{
}

bool LayoutObject::is_ancestor_of(const std::shared_ptr<LayoutObject> ancestor)
{
    // invalid LayoutItems can never be an ancestor
    if (!ancestor) {
        return false;
    }

    // check all actual ancestors against the candidate
    std::shared_ptr<LayoutObject> parent = get_parent();
    while (parent) {
        if (parent == ancestor) {
            return true;
        }
        parent = parent->get_parent();
    }

    // if no ancestor matches, we have our answer
    return false;
}

bool LayoutObject::has_child(const std::shared_ptr<LayoutObject> candidate)
{
    std::unique_ptr<ChildIteratorBase> it = get_child_iterator();
    while (const auto& child = it->next()) {
        if(child == candidate){
            return true;
        }
    }
    return false;
}

std::shared_ptr<const LayoutObject> LayoutObject::get_root_item() const
{
    std::shared_ptr<const LayoutObject> it = std::static_pointer_cast<const LayoutObject>(shared_from_this());
    while (it->has_parent()) {
        it = it->get_parent();
    }
    return it;
}

void LayoutObject::set_parent(std::shared_ptr<LayoutObject> parent)
{
    // do nothing if the new parent is the same as the old (or both are invalid)
    if (parent == get_parent()) {
        return;
    }

    // check for cyclic ancestry
    if (is_ancestor_of(parent)) {
        log_critical << "Cannot make " << parent->get_handle() << " the parent of " << get_handle()
                     << " because " << parent->get_handle() << " is already a child of " << get_handle();
        return;
    }

    m_parent = parent;
    parent_changed(m_parent);

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

void LayoutObject::change_visibility(const bool is_visible)
{
    // ignore non-changes
    if ((is_visible && m_visibility == VISIBILITY::VISIBLE) || (!is_visible && m_visibility == VISIBILITY::INVISIBLE)) {
        return;
    }

    // update visibility
    const VISIBILITY previous_visibility = m_visibility;
    if (is_visible) {
        if (std::shared_ptr<LayoutObject> parent = get_parent()) {
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

void LayoutObject::cascade_visibility(const VISIBILITY visibility)
{
    // update your own visibility
    if (visibility == m_visibility) {
        return;
    }
    m_visibility = visibility;
    visibility_changed(m_visibility);

    // update your children's visiblity
    std::unique_ptr<ChildIteratorBase> it = get_child_iterator();
    while (auto& child = it->next()) {
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
    if (std::shared_ptr<LayoutObject> parent = get_parent()) {
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
