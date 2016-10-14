#pragma once

#include <atomic>
#include <memory>
#include <unordered_map>

#include "common/handle.hpp"

namespace signal {

class LayoutItem;

class LayoutItemManager {

    friend class LayoutItem;
    friend class Window;
    friend class WindowWidget;
    friend class Widget; // TODO: too many friends

public: // methods
    /// \brief Default Constructor.
    /// \param reserve  How many LayoutItems to reserve space for initially.
    explicit LayoutItemManager(const size_t reserve = 1024);

    /// \brief Returns the next, free Handle.
    /// Is thread-safe.
    Handle get_next_handle()
    {
        Handle handle;
        do {
            handle = m_nextHandle++;
        } while (m_layout_items.count(handle));
        return handle;
    }

    /// \brief Checks if the the given Handle denotes a LayoutItem.
    bool has_item(const Handle handle) const { return m_layout_items.count(handle); }

    /// \brief Returns a LayoutItem by its Handle.
    /// \param handle   Handle associated with the requested LayoutItem.
    std::shared_ptr<LayoutItem> get_item(const Handle handle) const;

private: // methods for LayoutItem
    /// \brief Registers a new LayoutItem with the Manager.
    /// The handle of the LayoutItem may not be the BAD_HANDLE, nor may it have been used to register another LayoutItem.
    /// \param item LayoutItem to register with its handle.
    /// \return True iff the LayoutItem was successfully registered - false if its handle is already taken.
    bool register_item(std::shared_ptr<LayoutItem> item);

    /// \brief Removes the data block for a given LayoutItem.
    /// This function should only be called once, in the destructor of the LayoutItem.
    /// \param handle   Handle of the LayoutItem whose data to remove.
    void release_item(const Handle handle);

private: // fields
    /// \brief The next available handle, is ever-increasing.
    std::atomic<Handle> m_nextHandle;

    /// \brief All LayoutItems in the Application indexed by handle.
    std::unordered_map<Handle, std::weak_ptr<LayoutItem>> m_layout_items;
};

} // namespace signal
