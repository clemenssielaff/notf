#include "app/core/item.hpp"

#include <atomic>
#include <string>
#include <unordered_set>

#include "common/log.hpp"
//#include "core/controller.hpp"
//#include "core/item_container.hpp"
//#include "core/layout.hpp"
//#include "core/window_layout.hpp"

namespace { // anonymous
using namespace notf;

/** Returns the next available ItemID.
 * Is thread-safe and ever-increasing.
 */
ItemID next_id()
{
    static std::atomic<size_t> g_nextID(1);
    return g_nextID++;
}

} // namespace

namespace notf {

Item::Item(detail::ItemContainerPtr container)
    : m_children(std::move(container))
    , m_id(next_id())
    , m_window()
    , m_parent()
    , m_name(std::to_string(static_cast<size_t>(id())))
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

bool Item::has_child(const Item* child) const { return m_children->contains(child); }

bool Item::has_children() const { return !m_children->is_empty(); }

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
        parent = parent->parent();
    }
    return false;
}

Item* Item::common_ancestor(Item* other)
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
            first = first->parent();
            if (known_ancestors.count(first)) {
                return first;
            }
            known_ancestors.insert(first);
        }
        if (second) {
            second = second->parent();
            if (known_ancestors.count(second)) {
                return second;
            }
            known_ancestors.insert(second);
        }
    }
}

Layout* Item::layout() { return _first_ancestor<Layout>(); }

Controller* Item::controller() { return _first_ancestor<Controller>(); }

ScreenItem* Item::screen_item()
{
    ScreenItem* screen_item = dynamic_cast<ScreenItem*>(this);
    if (!screen_item) {
        screen_item = dynamic_cast<Controller*>(this)->root_item();
    }
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

    m_children->apply([window](Item* item) -> void { item->_set_window(window); });

    on_window_changed(m_window);
}

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
    m_children->apply([](Item* item) -> void { item->_update_from_parent(); });

    on_parent_changed(m_parent);
}

//====================================================================================================================//

namespace detail {

ItemContainer::~ItemContainer() {}

void ItemContainer::clear()
{
    apply([](Item* item) -> void { item->_set_parent(nullptr); });
}

void ItemContainer::destroy()
{
    apply([](Item* item) -> void { item->_set_parent(nullptr, /* is_orphaned = */ true); });
}

//====================================================================================================================//

EmptyItemContainer::~EmptyItemContainer() {}

//====================================================================================================================//

SingleItemContainer::~SingleItemContainer() { item.reset(); }

void SingleItemContainer::clear()
{
    ItemContainer::clear();
    item.reset();
}

void SingleItemContainer::apply(std::function<void(Item*)> function)
{
    if (item) {
        function(item.get());
    }
}

//====================================================================================================================//

ItemList::~ItemList() { items.clear(); }

void ItemList::clear()
{
    ItemContainer::clear();
    items.clear();
}

void ItemList::apply(std::function<void(Item*)> function)
{
    for (const ItemPtr& item : items) {
        function(item.get());
    }
}

bool ItemList::contains(const Item* child) const
{
    return std::find_if(std::begin(items), std::end(items),
                        [child](const ItemPtr& entry) -> bool { return entry.get() == child; })
           != std::end(items);
}

} // namespace detail

} // namespace notf
