#include "core/item.hpp"

#include <atomic>

#include "common/log.hpp"
#include "core/controller.hpp"
#include "core/layout.hpp"
#include "core/window_layout.hpp"
#include "core/render_manager.hpp"
#include "core/screen_item.hpp"
#include "core/window.hpp"

namespace { // anonymous
using namespace notf;

/** The next available Item ID, is ever-increasing. */
std::atomic<ItemID> g_nextID(1);

/** Returns the next, free ItemID.
 * Is thread-safe.
 */
ItemID get_next_id()
{
    return g_nextID++;
}

} // namespace anonymous

namespace notf {

ScreenItem* Item::get_screen_item(Item* item)
{
    ScreenItem* screen_item = dynamic_cast<ScreenItem*>(item);
    if (!screen_item) {
        Controller* controller = dynamic_cast<Controller*>(item);
        screen_item            = controller->get_root_item().get();
    }
    assert(screen_item);
    return screen_item;
}

Item::Item()
    : m_id(get_next_id())
    , m_parent()
    , m_render_layer() // empty by default
#ifdef NOTF_PYTHON
    , m_py_object(nullptr, py_decref)
#endif
{
    log_trace << "Created Item #" << m_id;
}

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
    const Item* my_parent = get_parent().get();
    while (my_parent) {
        if (my_parent == ancestor) {
            return true;
        }
        my_parent = my_parent->get_parent().get();
    }

    // if no ancestor matches, we have our answer
    return false;
}

std::shared_ptr<Window> Item::get_window() const
{
    if (std::shared_ptr<const WindowLayout> window_layout = _get_first_ancestor<WindowLayout>()) {
        return window_layout->get_window();
    }
    return {};
}

const std::shared_ptr<RenderLayer>& Item::get_render_layer() const
{
    // if you have your own RenderLayer, return that
    if (m_render_layer) {
        return m_render_layer;
    }

    // ... otherwise return the nearest ancestor's one
    const Item* ancestor = get_parent().get();
    while (ancestor) {
        if (const std::shared_ptr<RenderLayer>& render_layer = ancestor->get_render_layer()) {
            return render_layer;
        }
        ancestor = ancestor->get_parent().get();
    }

    // the WindowLayout at the latest should return a valid RenderLayer
    // but if this Item is not in the Item hierarchy, it is also not a part of a RenderLayer
    static const std::shared_ptr<RenderLayer> empty;
    return empty;
}

void Item::set_render_layer(std::shared_ptr<RenderLayer> render_layer)
{
    if (m_parent.expired()) {
        log_critical << "Cannot change the RenderLayer of an Item outside the Item hierarchy.";
        return;
    }
    m_render_layer = std::move(render_layer);
}

std::shared_ptr<Layout> Item::_get_layout() const
{
    return _get_first_ancestor<Layout>();
}

std::shared_ptr<Controller> Item::_get_controller() const
{
    return _get_first_ancestor<Controller>();
}

void Item::_update_parent_layout()
{
    std::shared_ptr<Layout> layout = _get_layout();
    while (layout) {
        // if the parent Layout's Claim changed, we also need to update its parent ...
        if (layout->_update_claim()) {
            layout = layout->_get_layout();
        }
        // ... otherwise, we have reached the end of the propagation through the ancestry
        // and continue to relayout all children from the parent downwards
        else {
            layout->_relayout();
            return;
        }
    }
}

template <typename AncestorType>
std::shared_ptr<AncestorType> Item::_get_first_ancestor() const
{
    std::shared_ptr<Item> next = m_parent.lock();
    while (next) {
        if (std::shared_ptr<AncestorType> result = std::dynamic_pointer_cast<AncestorType>(next)) {
            return result;
        }
        next = next->m_parent.lock();
    }
    return {};
}

#ifdef NOTF_PYTHON
void Item::_set_pyobject(PyObject* object)
{
    assert(!m_py_object); // you should only have to do this once
    py_incref(object);
    m_py_object.reset(std::move(object));
}
#endif

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

    // remove yourself from the old parent Layout
    if (std::shared_ptr<Layout> old_layout = std::dynamic_pointer_cast<Layout>(old_parent)) {
        old_layout->_remove_child(this);
    }

    m_parent = parent;
    parent_changed(parent->get_id());
}

} // namespace notf
