#include "core/object.hpp"

namespace notf {

Object::~Object()
{
    // release the manager's weak_ptr
    Application::get_instance().get_object_manager()._release_object(m_handle);
}

} // namespace notf
