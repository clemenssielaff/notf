#include "core/abstract_object.hpp"

#include "common/log.hpp"

namespace notf {

AbstractObject::~AbstractObject()
{
    // release the manager's weak_ptr
    Application::get_instance().get_item_manager().release_object(m_handle);

    log_trace << "Destroyed Item with handle:" << get_handle();
}

} // namespace notf
