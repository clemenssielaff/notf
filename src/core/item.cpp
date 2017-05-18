#include "core/item.hpp"

#include <atomic>

#include "common/log.hpp"
#include "core/controller.hpp"
#include "core/layout.hpp"
#include "core/render_manager.hpp"
#include "core/screen_item.hpp"
#include "core/window.hpp"
#include "core/window_layout.hpp"

namespace { // anonymous
using namespace notf;

/** The next available Item ID, is ever-increasing. */
std::atomic<RawID> g_nextID(1);

/** Returns the next, free ItemID.
 * Is thread-safe.
 */
ItemID get_next_id()
{
    return g_nextID++;
}

} // namespace anonymous

namespace notf {

Item::Item()
    : m_render_layer() // empty by default
    , m_id(get_next_id())
    , m_parent()
    , m_has_own_render_layer(false)
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

bool Item::has_ancestor(const ItemPtr& ancestor) const
{
    // invalid Item can never be an ancestor
    if (!ancestor) {
        return false;
    }

    // check all actual ancestors against the candidate
    ItemPtr my_parent = get_parent();
    while (my_parent) {
        if (my_parent == ancestor) {
            return true;
        }
        my_parent = my_parent->get_parent();
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

void Item::set_render_layer(const RenderLayerPtr& render_layer)
{
    ItemPtr parent = m_parent.lock();
    if (!parent) {
        throw std::runtime_error("Cannot change the RenderLayer of an Item outside the Item hierarchy.");
    }
    m_has_own_render_layer = false;
    if (render_layer) {
        _cascade_render_layer(render_layer);
        m_has_own_render_layer = true;
    }
    else {
        _cascade_render_layer(parent->m_render_layer);
    }
}

LayoutPtr Item::_get_layout() const
{
    return _get_first_ancestor<Layout>();
}

ControllerPtr Item::_get_controller() const
{
    return _get_first_ancestor<Controller>();
}

template <typename AncestorType>
std::shared_ptr<AncestorType> Item::_get_first_ancestor() const
{
    ItemPtr next = m_parent.lock();
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

void Item::_set_parent(ItemPtr parent)
{
    // do nothing if the new parent is the same as the old (or both are invalid)
    ItemPtr old_parent = m_parent.lock();
    if (parent == old_parent) {
        return;
    }

    // check for cyclic ancestry
    if (has_ancestor(parent)) {
        log_critical << "Cannot make " << parent->get_id() << " the parent of " << get_id()
                     << " because " << parent->get_id() << " is already a child of " << get_id();
        return;
    }

    // remove yourself from the old parent Layout
    if (LayoutPtr old_layout = std::dynamic_pointer_cast<Layout>(old_parent)) {
        old_layout->remove_item(shared_from_this());
    }

    // set the new parent and return if it is empty
    m_parent = parent;
    if (!parent) {
        on_parent_changed(0);
        return;
    }

    // inherit the parent's RenderLayer, if you don't have your own
    if (!_has_own_render_layer()) {
        _cascade_render_layer(parent->m_render_layer);
    }

    on_parent_changed(parent->get_id());
}

/**********************************************************************************************************************/

ScreenItem* get_screen_item(Item* item)
{
    if (!item) {
        return nullptr;
    }
    ScreenItem* screen_item = dynamic_cast<ScreenItem*>(item);
    if (!screen_item) {
        Controller* controller = dynamic_cast<Controller*>(item);
        screen_item            = controller->get_root_item().get();
    }
    assert(screen_item);
    return screen_item;
}

const Item* get_common_ancestor(const Item* i_first, const Item* i_second)
{
    if (i_first == i_second) {
        return i_first;
    }
    const Item* first  = i_first;
    const Item* second = i_second;

    std::set<const Item*> known_ancestors = {first, second};
    while (first && second) {
        first  = first->get_layout().get();
        second = second->get_layout().get();
        if (known_ancestors.count(first)) {
            return first;
        }
        if (known_ancestors.count(second)) {
            return second;
        }
        known_ancestors.insert(first);
        known_ancestors.insert(second);
    }

    return {};
}

} // namespace notf
