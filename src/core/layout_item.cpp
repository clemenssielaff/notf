#include "core/layout_item.hpp"

#include "common/log.hpp"
#include "core/layout.hpp"
#include "core/layout_root.hpp"
#include "core/render_manager.hpp"
#include "core/window.hpp"

namespace notf {

LayoutItem::~LayoutItem()
{
}

bool LayoutItem::has_ancestor(const LayoutItem* ancestor) const
{
    // invalid LayoutItem can never be an ancestor
    if (!ancestor) {
        return false;
    }

    // check all actual ancestors against the candidate
    const Layout* parent = get_parent().get();
    while (parent) {
        if (parent == ancestor) {
            return true;
        }
        parent = parent->get_parent().get();
    }

    // if no ancestor matches, we have our answer
    return false;
}

std::shared_ptr<Window> LayoutItem::get_window() const
{
    std::shared_ptr<const LayoutItem> it = std::static_pointer_cast<const LayoutItem>(shared_from_this());
    while (it->has_parent()) {
        it = it->get_parent();
    }
    std::shared_ptr<const LayoutRoot> root_item = std::dynamic_pointer_cast<const LayoutRoot>(it);
    if (!root_item) {
        return {};
    }
    return root_item->get_window();
}

LayoutItem::LayoutItem(const Handle handle)
    : Object(handle)
    , m_parent()
    , m_visibility(VISIBILITY::VISIBLE)
    , m_size()
    , m_transform(Transform2::identity())
    , m_render_layer() // empty by default
{
}

void LayoutItem::_set_visible(const bool is_visible)
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
                _cascade_visibility(VISIBILITY::HIDDEN);
            }
            else {
                _cascade_visibility(parent_visibility);
            }
        }
        else {
            _cascade_visibility(VISIBILITY::UNROOTED);
        }
    }
    else {
        _cascade_visibility(VISIBILITY::INVISIBLE);
    }

    // redraw if the object just became visible or invisible
    if (previous_visibility != m_visibility) {
        if (is_visible) {
            _redraw();
        }
        else {
            _update_parent_layout();
        }
    }
}

void LayoutItem::_update_parent_layout()
{
    std::shared_ptr<Layout> parent = m_parent.lock();
    while (parent) {
        // if the parent Layout's Claim changed, we also need to update its parent
        if(parent->_update_claim()){
            parent = parent->m_parent.lock();
        }
        // ... otherwise, we have reached the end of the propagation through the ancestry
        // and continue to relayout all children from the parent downwards
        else {
            parent->_relayout();
            parent.reset();
        }
    }
}

bool LayoutItem::_redraw()
{
    // do not request a redraw, if this item cannot be drawn anyway
    if (!is_visible()) {
        return false;
    }
    if (get_size().is_zero() && !get_size().is_valid()) {
        return false;
    }

    if(std::shared_ptr<Window> window = get_window()){
        window->get_render_manager().request_redraw();
    }
    return true;
}

void LayoutItem::_set_parent(std::shared_ptr<Layout> parent)
{
    // do nothing if the new parent is the same as the old (or both are invalid)
    std::shared_ptr<Layout> old_parent = m_parent.lock();
    if (parent == old_parent) {
        return;
    }

    // check for cyclic ancestry
    if (has_ancestor(parent.get())) {
        log_critical << "Cannot make " << parent->get_handle() << " the parent of " << get_handle()
                     << " because " << parent->get_handle() << " is already a child of " << get_handle();
        return;
    }

    // update your parent
    if (old_parent) {
        old_parent->_remove_child(this);
    }

    m_parent = parent;
    parent_changed(parent->get_handle());

    // update visibility
    if (!parent) {
        if (m_visibility != VISIBILITY::INVISIBLE) {
            _cascade_visibility(VISIBILITY::UNROOTED);
        }
    }
    else {
        VISIBILITY parent_visibility = parent->get_visibility();
        if (parent_visibility == VISIBILITY::INVISIBLE) {
            if (m_visibility != VISIBILITY::INVISIBLE) {
                _cascade_visibility(VISIBILITY::HIDDEN);
            }
        }
        else if (m_visibility != VISIBILITY::INVISIBLE) {
            _cascade_visibility(parent_visibility);
        }
    }
}

void LayoutItem::_cascade_visibility(const VISIBILITY visibility)
{
    // update your own visibility
    if (visibility == m_visibility) {
        return;
    }
    m_visibility = visibility;
    _redraw();
    visibility_changed(m_visibility);
}

void LayoutItem::_get_window_transform(Transform2& result) const
{
    if (std::shared_ptr<const Layout> parent = get_parent()) {
        parent->_get_window_transform(result);
    }
    result = m_transform * result;
}

Transform2 LayoutItem::_get_screen_transform() const
{
    log_critical << "get_transform(SPACE::SCREEN) is not emplemented yet";
    return _get_parent_transform();
}

} // namespace notf
