#include "core/object_manager.hpp"

#include <assert.h>

#include "core/object.hpp"

namespace notf {

ObjectManager::ObjectManager(const size_t reserve)
    : m_nextHandle(_FIRST_HANDLE)
    , m_object()
{
    m_object.reserve(reserve);
}

Handle ObjectManager::_get_next_handle()
{
    Handle handle;
    do {
        handle = m_nextHandle++;
    } while (m_object.count(handle));
    return handle;
}

bool ObjectManager::_register_object(std::shared_ptr<Object> object)
{
    // don't register the Object if its handle is already been used
    const Handle handle = object->get_handle();
    if (m_object.count(handle)) {
        log_critical << "Cannot register Object with handle " << handle
                     << " because the handle is already taken";
        return false;
    }
    m_object.emplace(std::make_pair(handle, object));
    log_trace << "Registered Object with handle " << handle;
    return true;
}

void ObjectManager::_release_object(const Handle handle)
{
    auto it = m_object.find(handle);
    if (it == m_object.end()) {
        log_critical << "Cannot release Object with unknown handle:" << handle;
    }
    else {
        m_object.erase(it);
        log_trace << "Releasing Object with handle " << handle;
    }
}

} // namespace notf
