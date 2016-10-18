#pragma once

#include <memory>

#include "common/handle.hpp"
#include "core/application.hpp"
#include "core/item_manager.hpp"
#include "utils/smart_enabler.hpp"

namespace signal {

/*
 * \brief AbstractItem is the base class for everything in an Application that can be accessible by a unique Handle.
 *
 * The memory of Items is always managed through std::shared_ptrs.
 * In fact, you cannot create one on the stack but must use the static `create()`-methods that Items generally offer.
 */
class AbstractItem : public std::enable_shared_from_this<AbstractItem> {

public: // methods
    /// no copy / assignment
    AbstractItem(const AbstractItem&) = delete;
    AbstractItem& operator=(const AbstractItem&) = delete;

    /// \brief Virtual destructor.
    virtual ~AbstractItem();

    /// \brief The Application-unique Handle of this LayoutItem.
    Handle get_handle() const { return m_handle; }

protected: // methods
    /// \brief Value Constructor.
    /// \param handle   Application-unique Handle of this Item.
    explicit AbstractItem(const Handle handle)
        : m_handle(handle)
    {
    }

protected: // static methods
    /// \brief Factory function to create a new Item.
    /// \param handle   [optional] Requested Handle of the new item - a new one is generated if BAD_HANDLE is passed.
    /// \return The created Item, is invalid if a requested Handle is already taken.
    template <typename T, typename = std::enable_if_t<std::is_base_of<AbstractItem, T>::value>, typename... ARGS>
    static std::shared_ptr<T> create_item(Handle handle, ARGS&&... args)
    {
        ItemManager& manager = Application::get_instance().get_item_manager();
        if (handle == BAD_HANDLE) {
            handle = manager.get_next_handle();
        }
        std::shared_ptr<T> item = std::make_shared<MakeSmartEnabler<T>>(handle, std::forward<ARGS>(args)...);
        if (!manager.register_item(item)) {
            return {};
        }
        return item;
    }

private: // fields
    /// \brief Application-unique Handle.
    const Handle m_handle;
};

} // namespace signal
