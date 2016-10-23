#include "core/layout_item.hpp"

#include "common/log.hpp"
#include "core/layout.hpp"
#include "core/layout_root.hpp"

namespace notf {

LayoutItem::~LayoutItem()
{
}

bool LayoutItem::has_ancestor(const std::shared_ptr<LayoutItem>& ancestor) const
{
    // invalid LayoutItem can never be an ancestor
    if (!ancestor) {
        return false;
    }

    // check all actual ancestors against the candidate
    std::shared_ptr<const Layout> parent = get_parent();
    while (parent) {
        if (parent == ancestor) {
            return true;
        }
        parent = parent->get_parent();
    }

    // if no ancestor matches, we have our answer
    return false;
}

std::shared_ptr<const LayoutRoot> LayoutItem::get_root() const
{
    std::shared_ptr<const LayoutItem> it = std::static_pointer_cast<const LayoutItem>(shared_from_this());
    while (it->has_parent()) {
        it = it->get_parent();
    }
    return std::dynamic_pointer_cast<const LayoutRoot>(it);
}

void LayoutItem::set_visible(const bool is_visible)
{
    // ignore non-changes
    if ((is_visible && m_visibility == VISIBILITY::VISIBLE) || (!is_visible && m_visibility == VISIBILITY::INVISIBLE)) {
        return;
    }

    // update visibility
    const VISIBILITY previous_visibility = m_visibility;
    if (is_visible) {
        if (std::shared_ptr<const Layout> parent = get_parent()) {
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

void LayoutItem::relayout_up()
{
    std::shared_ptr<Layout> parent = m_parent.lock();
    while (parent) {
        if (parent->relayout()) {
            parent = parent->m_parent.lock();
        }
        else {
            parent->relayout_down();
            parent.reset();
        }
    }
}

void LayoutItem::set_parent(std::shared_ptr<Layout> parent)
{
    // do nothing if the new parent is the same as the old (or both are invalid)
    std::shared_ptr<Layout> old_parent = m_parent.lock();
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

void LayoutItem::cascade_visibility(const VISIBILITY visibility)
{
    // update your own visibility
    if (visibility == m_visibility) {
        return;
    }
    m_visibility = visibility;
    visibility_changed(m_visibility);
}

void LayoutItem::get_window_transform(Transform2& result) const
{
    if (std::shared_ptr<const Layout> parent = get_parent()) {
        parent->get_window_transform(result);
    }
    result = m_transform * result;
}

Transform2 LayoutItem::get_screen_transform() const
{
    log_critical << "get_transform(SPACE::SCREEN) is not emplemented yet";
    return get_parent_transform();
}

} // namespace notf
