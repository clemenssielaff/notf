#include "core/item.hpp"

#include "common/log.hpp"
#include "core/layout.hpp"
#include "core/layout_root.hpp"
#include "core/render_manager.hpp"
#include "core/window.hpp"

namespace notf {

std::atomic<ItemID> Item::s_nextID(1);

Item::~Item()
{
    log_trace << "Destroying Item #" << m_id;
}

bool Item::has_ancestor(const Item* ancestor) const
{
    // invalid Item can never be an ancestor
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

std::shared_ptr<Window> Item::get_window() const
{
    std::shared_ptr<const Item> it = std::static_pointer_cast<const Item>(shared_from_this());
    while (it->has_parent()) {
        it = it->get_parent();
    }
    std::shared_ptr<const LayoutRoot> root_item = std::dynamic_pointer_cast<const LayoutRoot>(it);
    if (!root_item) {
        return {};
    }
    return root_item->get_window();
}

Item::Item()
    : m_id(_get_next_id())
    , m_parent()
    , m_render_layer() // empty by default
    , py_object(nullptr, py_decref)
{
    log_trace << "Creating Item #" << m_id;
}

bool Item::_redraw()
{
    // do not request a redraw, if this item cannot be drawn anyway
    if (!is_visible()) {
        return false;
    }

    if (std::shared_ptr<Window> window = get_window()) {
        window->get_render_manager().request_redraw();
    }
    return true;
}

void Item::_update_parent_layout()
{
    std::shared_ptr<Layout> parent = m_parent.lock();
    while (parent) {
        // if the parent Layout's Claim changed, we also need to update its parent
        if (parent->_update_claim()) {
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

void Item::_set_pyobject(PyObject* object)
{
    assert(!py_object); // you should only do this once
    py_incref(object);
    py_object.reset(std::move(object));
}

void Item::_set_parent(std::shared_ptr<Layout> parent)
{
    // do nothing if the new parent is the same as the old (or both are invalid)
    std::shared_ptr<Layout> old_parent = m_parent.lock();
    if (parent == old_parent) {
        return;
    }

    // check for cyclic ancestry
    if (has_ancestor(parent.get())) {
        log_critical << "Cannot make " << parent->get_id() << " the parent of " << get_id()
                     << " because " << parent->get_id() << " is already a child of " << get_id();
        return;
    }

    // update your parent
    if (old_parent) {
        old_parent->_remove_child(this);
    }

    m_parent = parent;
    parent_changed(parent->get_id());
}

void Item::_get_window_transform(Transform2& result) const
{
    if (std::shared_ptr<const Layout> parent = get_parent()) {
        parent->_get_window_transform(result);
    }
    result = _get_local_transform() * result;
}

Transform2 Item::_get_screen_transform() const
{
    log_critical << "get_transform(SPACE::SCREEN) is not emplemented yet";
    return _get_local_transform();
}

ItemID Item::_get_next_id()
{
    return s_nextID++;
}

} // namespace notf
