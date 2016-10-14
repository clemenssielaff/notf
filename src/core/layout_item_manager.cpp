#include "core/layout_item_manager.hpp"

#include "common/log.hpp"
#include "core/layout_item.hpp"

namespace signal {

LayoutItemManager::LayoutItemManager(const size_t reserve)
    : m_nextHandle(_FIRST_HANDLE)
    , m_layout_items()
{
    m_layout_items.reserve(reserve);
}

std::shared_ptr<LayoutItem> LayoutItemManager::get_item(const Handle handle) const
{
    auto it = m_layout_items.find(handle);
    if (it == m_layout_items.end()) {
        log_warning << "Requested LayoutItem with unknown handle:" << handle;
        return {};
    }

    std::shared_ptr<LayoutItem> layout_item = it->second.lock();
    if (!layout_item) {
        log_critical << "Requested LayoutItem with expired handle:" << handle;
        return {};
    }
    return layout_item;
}

bool LayoutItemManager::register_item(std::shared_ptr<LayoutItem> item)
{
    // don't register the LayoutItem if its handle is already been used
    if (m_layout_items.count(item->get_handle())) {
        return false;
    }
    m_layout_items.emplace(std::make_pair(item->get_handle(), item));
    return true;
}

void LayoutItemManager::release_item(const Handle handle)
{
    auto it = m_layout_items.find(handle);
    if (it == m_layout_items.end()) {
        log_critical << "Cannot remove LayoutItem with unknown handle:" << handle;
    }
    else {
        m_layout_items.erase(it);
    }
}

} // namespace signal
