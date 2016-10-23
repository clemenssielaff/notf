#include "core/layout.hpp"

#include "common/log.hpp"

namespace notf {

Layout::~Layout()
{
    // explicitly unparent all children so they can send the `parent_changed` signal
    for (auto& it : m_children) {
        it.second->unparent();
    }
}

bool Layout::has_child(const std::shared_ptr<LayoutItem>& candidate) const
{
    for (const auto& it : m_children) {
        if (it.second == candidate) {
            return true;
        }
    }
    return false;
}

std::shared_ptr<LayoutItem> Layout::get_child(const Handle child_handle) const
{
    auto it = m_children.find(child_handle);
    if (it == m_children.end()) {
        log_warning << "Requested unknown child" << child_handle << " from LayoutItem " << get_handle();
        return {};
    }
    return it->second;
}

void Layout::add_child(std::shared_ptr<LayoutItem> child_object)
{
    const Handle child_handle = child_object->get_handle();
    if (m_children.count(child_handle)) {
        log_warning << "Did not add existing child " << child_handle << " to LayoutItem " << get_handle();
        return;
    }
    child_object->set_parent(std::static_pointer_cast<Layout>(shared_from_this()));
    m_children.emplace(std::make_pair(child_handle, std::move(child_object)));
    child_added(child_handle);
}

void Layout::remove_child(const Handle child_handle)
{
    auto it = m_children.find(child_handle);
    if (it == m_children.end()) {
        log_critical << "Failed to remove unknown child " << child_handle << " from LayoutItem " << get_handle();
    }
    else {
        m_children.erase(it);
        child_removed(child_handle);
    }
}

void Layout::update_child_layouts()
{
    for (const auto& it : m_children) {
        if (std::shared_ptr<Layout> layout = std::dynamic_pointer_cast<Layout>(it.second)) {
            if (layout->relayout()) {
                layout->update_child_layouts();
            }
        }
    }
}

void Layout::cascade_visibility(const VISIBILITY visibility)
{
    if (visibility == get_visibility()) {
        return;
    }
    LayoutItem::cascade_visibility(visibility);

    // update your children's visiblity
    for (auto& it : m_children) {
        if (it.second->get_visibility() != VISIBILITY::INVISIBLE) {
            if (get_visibility() == VISIBILITY::INVISIBLE) {
                it.second->cascade_visibility(VISIBILITY::HIDDEN);
            }
            else {
                it.second->cascade_visibility(visibility);
            }
        }
    }
}

void Layout::redraw()
{
    // don't draw invisible objects
    if (get_visibility() != VISIBILITY::VISIBLE) {
        return;
    }

    // redraw all children
    for (auto& it : m_children) {
        it.second->redraw();
    }
}

} // namespace notf
