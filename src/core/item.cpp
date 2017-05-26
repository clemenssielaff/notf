#include "core/item.hpp"

#include <atomic>
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

Item::Item(std::unique_ptr<detail::ItemContainer> container)
    : m_children(std::move(container))
    , m_id(get_next_id())
    , m_window()
    , m_parent()
#ifdef NOTF_PYTHON
    , m_py_object(nullptr, py_decref)
#endif
{
    log_trace << "Created Item #" << m_id;
}

Item::~Item()
{
    log_trace << "Destroying Item #" << m_id;
    m_children->clear();
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

void Item::_set_parent(Item* parent)
{
    if (parent == m_parent) {
        return;
    }
    m_parent = parent;

    if (m_parent) {
        _set_window(m_parent->m_window);
    }
    else {
        _set_window(nullptr);
    }

    on_parent_changed(m_parent);
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

} // namespace notf
