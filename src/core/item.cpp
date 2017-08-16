#include "core/item.hpp"

#include <atomic>
#include <string>
#include <unordered_set>

#include "common/log.hpp"
#include "core/controller.hpp"
#include "core/item_container.hpp"
#include "core/layout.hpp"
#include "core/window_layout.hpp"

namespace { // anonymous
using namespace notf;

/** Returns the next available ItemID.
 * Is thread-safe and ever-increasing.
 */
ItemID get_next_id()
{
    static std::atomic<RawID> g_nextID(1);
    return g_nextID++;
}

} // namespace anonymous

namespace notf {

Item::Item(ItemContainerPtr container)
    : m_children(std::move(container))
    , m_id(get_next_id())
    , m_window()
    , m_parent()
    , m_name(std::to_string(static_cast<size_t>(get_id())))
#ifdef NOTF_PYTHON
    , m_py_object(nullptr, py_decref)
#endif
{
    log_trace << "Created Item #" << m_id;
}

Item::~Item()
{
    log_trace << "Destroying Item #" << m_id;
    m_children->destroy();
    m_children.reset();
    if (m_parent) {
        m_parent->_remove_child(this);
    }
}

bool Item::has_child(const Item* child) const
{
    return m_children->contains(child);
}

bool Item::has_children() const
{
    return !m_children->is_empty();
}

bool Item::has_ancestor(const Item* ancestor) const
{
    if (!ancestor) {
        return false;
    }

    const Item* parent = m_parent;
    while (parent) {
        if (parent == ancestor) {
            return true;
        }
        parent = parent->get_parent();
    }
    return false;
}

Item* Item::get_common_ancestor(Item* other)
{
    if (this->m_window != other->m_window) {
        return nullptr;
    }

    if (this == other) {
        return this;
    }

    Item* first  = this;
    Item* second = other;

    std::unordered_set<Item*> known_ancestors = {first, second};
    while (1) {
        if (first) {
            first = first->get_parent();
            if (known_ancestors.count(first)) {
                return first;
            }
            known_ancestors.insert(first);
        }
        if (second) {
            second = second->get_parent();
            if (known_ancestors.count(second)) {
                return second;
            }
            known_ancestors.insert(second);
        }
    }
}

Layout* Item::get_layout()
{
    return _get_first_ancestor<Layout>();
}

Controller* Item::get_controller()
{
    return _get_first_ancestor<Controller>();
}

ScreenItem* Item::get_screen_item()
{
    ScreenItem* screen_item = dynamic_cast<ScreenItem*>(this);
    if (!screen_item) {
        screen_item = dynamic_cast<Controller*>(this)->get_root_item();
    }
    assert(screen_item);
    return screen_item;
}

void Item::_update_from_parent()
{
    if (m_parent) {
        _set_window(m_parent->m_window);
    }
    else {
        _set_window(nullptr);
    }
}

void Item::_set_window(Window* window)
{
    if (window == m_window) {
        return;
    }
    m_window = window;

    m_children->apply([window](Item* item) -> void {
        item->_set_window(window);
    });

    on_window_changed(m_window);
}

template <typename Type>
Type* Item::_get_first_ancestor() const
{
    Item* next = m_parent;
    while (next) {
        if (Type* result = dynamic_cast<Type*>(next)) {
            return result;
        }
        next = next->m_parent;
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

void Item::_set_parent(Item* parent, bool is_orphaned)
{
    if (parent == m_parent) {
        return;
    }

    if (m_parent && !is_orphaned) {
        m_parent->_remove_child(this);
    }
    m_parent = parent;

    _update_from_parent();
    m_children->apply([](Item* item) -> void {
        item->_update_from_parent();
    });

    on_parent_changed(m_parent);
}

} // namespace notf
