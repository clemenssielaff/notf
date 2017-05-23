#include "core/item_container.hpp"

#include "core/item_new.hpp"

namespace notf {
namespace detail {

ItemContainer::~ItemContainer()
{
}

void ItemContainer::clear()
{
    apply([](Item* item) -> void {
        item->_set_parent(nullptr);
    });
}

void ItemContainer::apply_recursive(std::function<void(Item*)> function)
{
    // all top-level Items in the container first
    apply(function);

    // then their child Items
    apply([function](Item* item) -> void {
        item->m_children->apply_recursive(function);
    });
}

/**********************************************************************************************************************/

void SingleItemContainer::apply(std::function<void(Item*)> function)
{
    if (m_item) {
        function(m_item.get());
    }
}

/**********************************************************************************************************************/

void ItemList::apply(std::function<void(Item*)> function)
{
    for (const ItemPtr& item : m_items) {
        function(item.get());
    }
}

} // namespace detail
} // namespace notf
