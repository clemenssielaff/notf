#include "app/core/controller.hpp"

#include "app/core/screen_item.hpp"
#include "common/log.hpp"

namespace notf {

Controller::Controller() : Item(std::make_unique<detail::SingleItemContainer>()), m_root_item(nullptr) {}

void Controller::_set_root_item(const ScreenItemPtr& item)
{
    if (m_root_item) {
        _remove_child(m_root_item);
    }
    detail::SingleItemContainer* child = static_cast<detail::SingleItemContainer*>(m_children.get());
    child->item                        = item;
    m_root_item                        = item.get();
    if (m_root_item) {
        Item::_set_parent(m_root_item, this);
    }
}

void Controller::_remove_child(const Item* child_item)
{
    if (child_item != m_root_item) {
        log_critical << "Cannot remove unknown child Item " << child_item->name() << " from Controller " << name();
        return;
    }

    log_trace << "Removing root item from Controller " << name();
    detail::SingleItemContainer* child = static_cast<detail::SingleItemContainer*>(m_children.get());
    child->item.reset();
    m_root_item = nullptr;
}

} // namespace notf
