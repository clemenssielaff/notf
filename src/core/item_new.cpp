#include "core/item_new.hpp"

#include <atomic>
#include <unordered_set>

#include "common/log.hpp"
#include "core/controller.hpp"
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

Item::Item()
    : m_id(get_next_id())
    , m_parent()
    , m_raw_parent()
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
    if (!ancestor) {
        return false;
    }

    ConstItemPtr parent = get_parent();
    while (parent) {
        if (parent.get() == ancestor) {
            return true;
        }
        parent = parent->get_parent();
    }
    return false;
}

ItemPtr Item::get_common_ancestor(Item* other)
{
    if (this == other) {
        return shared_from_this();
    }

    ItemPtr first  = shared_from_this();
    ItemPtr second = other->shared_from_this();

    std::unordered_set<ItemPtr> known_ancestors = {first, second};
    while (first && second) {
        first  = first->get_parent();
        second = second->get_parent();
        if (known_ancestors.count(first)) {
            return first;
        }
        if (known_ancestors.count(second)) {
            return second;
        }
        known_ancestors.insert(first);
        known_ancestors.insert(second);
    }

    return nullptr;
}

LayoutPtr Item::get_layout()
{
    return _get_first_ancestor<Layout>();
}

ControllerPtr Item::get_controller()
{
    return _get_first_ancestor<Controller>();
}

ScreenItemPtr Item::get_screen_item()
{
    ScreenItemPtr screen_item = std::dynamic_pointer_cast<ScreenItem>(shared_from_this());
    if (!screen_item) {
        screen_item = dynamic_cast<Controller*>(this)->get_root_item();
    }
    assert(screen_item);
    return screen_item;
}

Window* Item::get_window() const
{
    if (WindowLayoutPtr window_layout = _get_first_ancestor<WindowLayout>()) {
        return window_layout->get_window();
    }
    return {};
}

template <typename Type>
std::shared_ptr<Type> Item::_get_first_ancestor() const
{
    ItemPtr next = m_parent.lock();
    while (next) {
        if (std::shared_ptr<Type> result = std::dynamic_pointer_cast<Type>(next)) {
            return result;
        }
        next = next->m_parent.lock();
    }
    return {};
}

} // namespace notf
