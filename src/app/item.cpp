#include "app/item.hpp"

#include <atomic>
#include <unordered_set>

#include "app/application.hpp"
#include "common/log.hpp"
#include "common/vector.hpp"

namespace { // anonymous
NOTF_USING_NAMESPACE

/// Returns the next available ItemID.
/// Is thread-safe and ever-increasing.
ItemId next_id()
{
    static std::atomic<ItemId::underlying_t> g_nextID(1);
    return g_nextID++;
}

} // namespace

NOTF_OPEN_NAMESPACE

//====================================================================================================================//

Item::ChildContainer::~ChildContainer() {}

//====================================================================================================================//

Item::Item(const Token&, std::unique_ptr<ChildContainer> container) : m_children(std::move(container)), m_id(next_id())
{
    log_trace << "Created Item #" << m_id;
}

Item::~Item()
{
    if (m_name.empty()) {
        log_trace << "Destroying Item #" << m_id;
    }
    else {
        log_trace << "Destroying Item \"" << m_name << "\"";
    }

    m_children->_destroy();
    m_children.reset();

    if (m_parent) {
        m_parent->_remove_child(this);
    }
    m_parent = nullptr;
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

    Item* first = this;
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

const std::string& Item::set_name(std::string name)
{
    if (name != m_name) {
        m_name = std::move(name);
        on_name_changed(m_name);
    }
    return m_name;
}

void Item::_set_parent(Item* parent, bool notify_old)
{
    if (parent == m_parent) {
        return;
    }

    if (m_parent && notify_old) {
        m_parent->_remove_child(this);
    }
    m_parent = parent;

    _update_from_parent();
    for (Item* item : *m_children) {
        item->_update_from_parent();
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
