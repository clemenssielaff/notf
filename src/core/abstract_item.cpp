#include "core/abstract_item.hpp"

#include "common/log.hpp"

namespace notf {

AbstractItem::~AbstractItem()
{
    // release the manager's weak_ptr
    Application::get_instance().get_item_manager().release_item(m_handle);

    log_trace << "Destroyed Item with handle:" << get_handle();
}

} // namespace notf
