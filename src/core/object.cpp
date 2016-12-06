#include "core/object.hpp"

#include "core/application.hpp"
#include "core/object_manager.hpp"
#include "utils/make_smart_enabler.hpp"

namespace notf {

#define OBJECT_MANAGER Application::get_instance().get_object_manager()

Object::Object()
    : m_handle(OBJECT_MANAGER._get_next_handle())
{
    // TODO: the ObjectManager approach is seriously broken right now
    //OBJECT_MANAGER._register_object(shared_from_this());
}

Object::~Object()
{
    OBJECT_MANAGER._release_object(m_handle);
}

} // namespace notf
