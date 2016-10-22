#include "core/object_manager.hpp"

#include <assert.h>

#include "common/log.hpp"
#include "core/abstract_object.hpp"

namespace notf {

ObjectManager::ObjectManager(const size_t reserve)
    : m_nextHandle(_FIRST_HANDLE)
    , m_object()
{
    m_object.reserve(reserve);
}

Handle ObjectManager::get_next_handle()
{
    Handle handle;
    do {
        handle = m_nextHandle++;
    } while (m_object.count(handle));
    return handle;
}

bool ObjectManager::register_object(std::shared_ptr<AbstractObject> object)
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

void ObjectManager::release_object(const Handle handle)
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

std::shared_ptr<AbstractObject> ObjectManager::get_abstract_object(const Handle handle) const
{
    auto it = m_object.find(handle);
    if (it == m_object.end()) {
        log_warning << "Requested Object with unknown handle:" << handle;
        return {};
    }
    std::shared_ptr<AbstractObject> result = it->second.lock();
    if (!result) {
        log_warning << "Encountered expired Object with handle:" << handle;
    }
    return result;
}

void ObjectManager::wrong_type_warning(const std::string& type_name, const Handle handle) const
{
#if SIGNAL_LOG_LEVEL <= SIGNAL_LOG_LEVEL_WARNING
    auto it = m_object.find(handle);
    assert(it != m_object.end());
    auto object = it->second.lock();
    assert(object);

    log_warning << "Requested handle " << handle << " as type \"" << type_name << "\" "
                << "but the Object is a \"" << typeid(*object).name() << "\"";
#endif
}

} // namespace notf
