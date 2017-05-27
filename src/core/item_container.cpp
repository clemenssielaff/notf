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

/**********************************************************************************************************************/

void SingleItemContainer::apply(std::function<void(Item*)> function)
{
    if (item) {
        function(item.get());
    }
}

/**********************************************************************************************************************/

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
