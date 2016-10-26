#include "core/object.hpp"

#include "common/log.hpp"

namespace notf {

Object::~Object()
{
    // release the manager's weak_ptr
    Application::get_instance().get_item_manager()._release_object(m_handle);

    log_trace << "Destroyed Item with handle:" << get_handle();
}

} // namespace notf
