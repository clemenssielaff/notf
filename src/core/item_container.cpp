#include "core/item_container.hpp"

#include <algorithm>

#include "core/item.hpp"

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

void ItemContainer::destroy()
{
    apply([](Item* item) -> void {
        item->_set_parent(nullptr, /* is_orphaned */ true);
    });
}

/**********************************************************************************************************************/

EmptyItemContainer::~EmptyItemContainer()
{
}

/**********************************************************************************************************************/

SingleItemContainer::~SingleItemContainer()
{
    item.reset();
}

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

/**********************************************************************************************************************/

ItemList::~ItemList()
{
    items.clear();
}

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
                        [child](const ItemPtr& entry) -> bool {
                            return entry.get() == child;
                        })
        != std::end(items);
}

} // namespace detail
} // namespace notf
