#include "core/controller.hpp"

#include "common/log.hpp"
#include "core/item_container.hpp"
#include "core/screen_item.hpp"
#include "dynamic/layout/overlayout.hpp"

namespace notf {

Controller::Controller()
    : Item(std::make_unique<detail::SingleItemContainer>())
    , m_root_item(nullptr)
{
}

void Controller::_set_root_item(ScreenItemPtr item)
{
    detail::SingleItemContainer* container = static_cast<detail::SingleItemContainer*>(m_children.get());
    container->item = item;
    m_root_item = item.get();
    if (m_root_item) {
        Item::_set_parent(m_root_item, this);
    }
}

void Controller::_remove_child(const Item* child_item)
{
    if (child_item != m_root_item) {
        log_critical << "Cannot remove unknown child Item " << child_item->get_id()
                     << " from Controller " << get_id();
        return;
    }

    log_trace << "Removing root item from Controller " << get_id();
    m_children->clear();
    m_root_item = nullptr;
}

} // namespace notf
