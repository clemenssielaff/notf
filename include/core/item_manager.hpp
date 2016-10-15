#pragma once

#include <atomic>
#include <memory>
#include <string>
#include <unordered_map>

#include "common/handle.hpp"

namespace signal {

class AbstractItem;

/*
 * \brief Mangager of everything in an Application that is accessible through its unique Handle.
 */
class ItemManager {

    friend class AbstractItem;

public: // methods
    /// \brief Default Constructor.
    /// \param reserve  How many Items to reserve space for initially.
    explicit ItemManager(const size_t reserve = 1024);

    /// \brief Checks if the the given Handle denotes a known Item.
    bool has_item(const Handle handle) const { return m_items.count(handle); }

    /// \brief Returns an Item by its Handle.
    /// The returned pointer is invalid if the handle does not identify an Item or the Item is of the wrong type.
    /// \param handle   Handle associated with the requested Item.
    /// \return Shared pointer to the item, is invalid on error.
    template <typename T, typename = typename std::enable_if<std::is_base_of<AbstractItem, T>::value>::type>
    std::shared_ptr<T> get_item(const Handle handle) const
    {
        std::shared_ptr<AbstractItem> abstract_item = get_abstract_item(handle);
        if (!abstract_item) {
            return {};
        }
        std::shared_ptr<T> cast_item = std::dynamic_pointer_cast<T>(abstract_item);
        if (!cast_item) {
            wrong_type_warning(typeid(T).name(), handle);
        }
        return cast_item;
    }

private: // methods for AbstractItem
    /// \brief Returns the next, free Handle.
    /// Is thread-safe.
    Handle get_next_handle();

    /// \brief Registers a new Item with the Manager.
    /// The handle of the Item may not be the BAD_HANDLE, nor may it have been used to register another Item.
    /// \param item Item to register with its handle.
    /// \return True iff the Item was successfully registered - false if its handle is already taken.
    bool register_item(std::shared_ptr<AbstractItem> item);

    /// \brief Removes the weak_ptr to a given Item when it is deleted.
    /// \param handle   Handle of the Item to release.
    void release_item(const Handle handle);

private: // methods
    /// \brief Accessor method providing the opportunity for logging.
    std::shared_ptr<AbstractItem> get_abstract_item(const Handle handle) const;

    /// \brief Wrapper for logging.
    void wrong_type_warning(const std::string& type_name, const Handle handle) const;

private: // fields
    /// \brief The next available handle, is ever-increasing.
    std::atomic<Handle> m_nextHandle;

    /// \brief All Items in the Application accessible by its unique Handle.
    std::unordered_map<Handle, std::weak_ptr<AbstractItem>> m_items;
};

} // namespace signal
