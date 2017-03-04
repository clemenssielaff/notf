#include "core/item.hpp"

#include "common/log.hpp"
#include "core/controller.hpp"
#include "core/layout.hpp"
#include "core/layout_root.hpp"

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
    const Item* parent = get_parent().get();
    while (parent) {
        if (parent == ancestor) {
            return true;
        }
        parent = parent->get_parent().get();
    }

    // if no ancestor matches, we have our answer
    return false;
}

std::shared_ptr<AbstractController> Item::get_controller() const
{
    return _get_first_ancestor<AbstractController>();
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
#ifdef NOTF_PYTHON
    , m_py_object(nullptr, py_decref)
#endif
{
    log_trace << "Creating Item #" << m_id;
}

template <typename T>
std::shared_ptr<T> Item::_get_first_ancestor() const
{
    std::shared_ptr<Item> next = m_parent.lock();
    while (next) {
        if (std::shared_ptr<T> result = std::dynamic_pointer_cast<T>(next)) {
            return result;
        }
        next = next->m_parent.lock();
    }
    return {};
}

void Item::_update_parent_layout()
{
    std::shared_ptr<Layout> parentLayout = _get_first_ancestor<Layout>();
    while (parentLayout) {
        // if the parent Layout's Claim changed, we also need to update its parent
        if (parentLayout->_update_claim()) {
            parentLayout = parentLayout->_get_first_ancestor<Layout>();
        }
        // ... otherwise, we have reached the end of the propagation through the ancestry
        // and continue to relayout all children from the parent downwards
        else {
            parentLayout->_relayout();
            parentLayout.reset();
        }
    }
}

#ifdef NOTF_PYTHON
void Item::_set_pyobject(PyObject* object)
{
    assert(!m_py_object); // you should only have to do this once
    py_incref(object);
    m_py_object.reset(std::move(object));
}
#endif

bool Item::_set_item_size(LayoutItem* layout_item, const Size2f& size)
{
    return layout_item->_set_size(size);
}

bool Item::_set_item_transform(LayoutItem* layout_item, const Transform2 transform)
{
    return layout_item->_set_transform(std::move(transform));
}

void Item::_set_parent(std::shared_ptr<Item> parent)
{
    // do nothing if the new parent is the same as the old (or both are invalid)
    std::shared_ptr<Item> old_parent = m_parent.lock();
    if (parent == old_parent) {
        return;
    }

    // check for cyclic ancestry
    if (has_ancestor(parent.get())) {
        log_critical << "Cannot make " << parent->get_id() << " the parent of " << get_id()
                     << " because " << parent->get_id() << " is already a child of " << get_id();
        return;
    }

    // if the old parent was a Layout, remove yourself from the Layout explicitly
    if (std::shared_ptr<Layout> old_layout = std::dynamic_pointer_cast<Layout>(old_parent)) {
        old_layout->_remove_child(this);
    }

    m_parent = parent;
    parent_changed(parent->get_id());
}

ItemID Item::_get_next_id()
{
    return s_nextID++;
}

} // namespace notf
