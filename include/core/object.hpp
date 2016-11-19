#pragma once

#include <memory>

#include "common/handle.hpp"
#include "core/application.hpp"
#include "core/object_manager.hpp"
#include "utils/smart_enabler.hpp"

namespace notf {

/*
 * @brief Object is the base class for everything in an Application that can be accessible by a unique Handle.
 *
 * The memory of Objects is always managed through std::shared_ptrs.
 * In fact, you cannot create one on the stack but must use the static `create()`-methods that Objects generally offer.
 */
class Object : public std::enable_shared_from_this<Object> {

public: // methods
    /// no copy / assignment
    Object(const Object&) = delete;
    Object& operator=(const Object&) = delete;

    /// @brief Virtual destructor.
    virtual ~Object();

    /// @brief The Application-unique Handle of this Object.
    Handle get_handle() const { return m_handle; }

protected: // methods
    /// @brief Value Constructor.
    /// @param handle   Application-unique Handle of this Object.
    explicit Object(const Handle handle)
        : m_handle(handle)
    {
    }

protected: // static methods
    /// @brief Factory function to create a new Object.
    /// @param handle   [optional] Requested Handle of the new Object - a new one is generated if BAD_HANDLE is passed.
    /// @return The created Object, is invalid if a requested Handle is already taken.
    template <typename T, typename = std::enable_if_t<std::is_base_of<Object, T>::value>, typename... ARGS>
    static std::shared_ptr<T> _create_object(Handle handle, ARGS&&... args)
    {
        ObjectManager& manager = Application::get_instance().get_object_manager();
        if (handle == BAD_HANDLE) {
            handle = manager._get_next_handle();
        }
        std::shared_ptr<T> object = std::make_shared<MakeSmartEnabler<T>>(handle, std::forward<ARGS>(args)...);
        if (!manager._register_object(object)) {
            return {};
        }
        return object;
    }
    // TODO: maybe the idea of manually assigning handles is not that great ... (see below for discussion) .. or they might be
    // The user needs a way to retrieve LayoutItems anyway, meaning each has a unique name (string)
    // but I guess internally, they might have a Handle - but then again, weak_ptr would usually be preferrable
    // since even though they might be more costly to copy, they will usually be created once and that's it
    // handles may be cheap to copy, but looking them up will not be cheaper than seeing if a weak_ptr is still alive
    // so, we might just get rid of handles for good then?

private: // fields
    /// @brief Application-unique Handle.
    const Handle m_handle;
};

} // namespace notf
