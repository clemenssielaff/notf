#include "app/scene/widget/item.hpp"

#include <atomic>
#include <unordered_set>

#include "app/scene/widget/controller.hpp"
#include "app/scene/widget/layout.hpp"
#include "common/log.hpp"

namespace { // anonymous
NOTF_USING_NAMESPACE

/// Returns the next available ItemID.
/// Is thread-safe and ever-increasing.
ItemID next_id()
{
    static std::atomic<size_t> g_nextID(1);
    return g_nextID++;
}

} // namespace

NOTF_OPEN_NAMESPACE

item_hierarchy_error::~item_hierarchy_error() = default;

//====================================================================================================================//

Item::ChildContainer::~ChildContainer() {}

//====================================================================================================================//

Item::Item(std::unique_ptr<ChildContainer> container)
    : m_children(std::move(container)), m_id(next_id()), m_parent(), m_name(std::to_string(static_cast<size_t>(id())))
{
    log_trace << "Created Item #" << m_id;
}

Item::~Item()
{
    log_trace << "Destroying Item #" << m_id;
    m_children->_destroy();
    m_children.reset();
    if (m_parent) {
        m_parent->_remove_child(this);
    }
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
        parent = parent->m_parent;
    }
    return false;
}

risky_ptr<Item> Item::common_ancestor(Item* other)
{
    if (this == other) {
        return this;
    }

    Item* first  = this;
    Item* second = other;

    std::unordered_set<Item*> known_ancestors = {first, second};
    while (1) {
        if (first) {
            first = first->m_parent;
            if (known_ancestors.count(first)) {
                return first;
            }
            known_ancestors.insert(first);
        }
        if (second) {
            second = second->m_parent;
            if (known_ancestors.count(second)) {
                return second;
            }
            known_ancestors.insert(second);
        }
    }
}

risky_ptr<Layout> Item::layout() { return _first_ancestor<Layout>(); }

risky_ptr<Controller> Item::controller() { return _first_ancestor<Controller>(); }

risky_ptr<ScreenItem> Item::screen_item()
{
    if (ScreenItem* screen_item = dynamic_cast<ScreenItem*>(this)) {
        return screen_item;
    }
    else {
        return dynamic_cast<Controller*>(this)->root_item();
    }
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
    for (Item& item : *m_children) {
        item._update_from_parent();
    }

    on_parent_changed(m_parent);
}

//====================================================================================================================//
namespace detail {

EmptyItemContainer::~EmptyItemContainer() {}

SingleItemContainer::~SingleItemContainer() { item.reset(); }

ItemList::~ItemList() { items.clear(); }

} // namespace detail

NOTF_CLOSE_NAMESPACE
