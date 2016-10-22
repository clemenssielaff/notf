#include "core/item_manager.hpp"

#include <assert.h>

#include "common/log.hpp"
#include "core/abstract_item.hpp"

namespace notf {

ItemManager::ItemManager(const size_t reserve)
    : m_nextHandle(_FIRST_HANDLE)
    , m_items()
{
    m_items.reserve(reserve);
}

Handle ItemManager::get_next_handle()
{
    Handle handle;
    do {
        handle = m_nextHandle++;
    } while (m_items.count(handle));
    return handle;
}

bool ItemManager::register_item(std::shared_ptr<AbstractItem> item)
{
    // don't register the Item if its handle is already been used
    const Handle handle = item->get_handle();
    if (m_items.count(handle)) {
        log_critical << "Cannot register Item with handle " << handle
                     << " because the handle is already taken";
        return false;
    }
    m_items.emplace(std::make_pair(handle, item));
    log_trace << "Registered Item with handle " << handle;
    return true;
}

void ItemManager::release_item(const Handle handle)
{
    auto it = m_items.find(handle);
    if (it == m_items.end()) {
        log_critical << "Cannot release Item with unknown handle:" << handle;
    }
    else {
        m_items.erase(it);
        log_trace << "Releasing Item with handle " << handle;
    }
}

std::shared_ptr<AbstractItem> ItemManager::get_abstract_item(const Handle handle) const
{
    auto it = m_items.find(handle);
    if (it == m_items.end()) {
        log_warning << "Requested Item with unknown handle:" << handle;
        return {};
    }
    std::shared_ptr<AbstractItem> result = it->second.lock();
    if (!result) {
        log_warning << "Encountered expired Item with handle:" << handle;
    }
    return result;
}

void ItemManager::wrong_type_warning(const std::string& type_name, const Handle handle) const
{
#if SIGNAL_LOG_LEVEL <= SIGNAL_LOG_LEVEL_WARNING
    auto it = m_items.find(handle);
    assert(it != m_items.end());
    auto item = it->second.lock();
    assert(item);

    log_warning << "Requested handle " << handle << " as type \"" << type_name << "\" "
                << "but the Item is a \"" << typeid(*item).name() << "\"";
#endif
}

} // namespace notf
