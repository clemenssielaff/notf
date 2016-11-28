#pragma once

#include <atomic>
#include <memory>
#include <string>
#include <unordered_map>

#include "common/handle.hpp"
#include "common/log.hpp"

namespace notf {

class Object;

/*
 * @brief Mangager of everything in an Application that is accessible through its unique Handle.
 */
class ObjectManager {

    friend class Object;

public: // methods
    /// @brief Default Constructor.
    /// @param reserve  How many Objects to reserve space for initially.
    explicit ObjectManager(const size_t reserve = 1024);

    /// @brief Checks if the the given Handle denotes a known Object.
    bool has_object(const Handle handle) const { return m_object.count(handle); }

    /// @brief Returns an Object by its Handle.
    /// The returned pointer is invalid if the handle does not identify an Object or the Object is of the wrong type.
    /// @param handle   Handle associated with the requested Object.
    /// @return Shared pointer to the Object, is invalid on error.
    template <typename T, typename = typename std::enable_if<std::is_base_of<Object, T>::value>::type>
    std::shared_ptr<T> get_object(const Handle handle) const
    {
        auto it = m_object.find(handle);
        if (it == m_object.end()) {
            log_warning << "Requested Object with unknown handle:" << handle;
            return {};
        }
        std::shared_ptr<Object> abstract_object = it->second.lock();
        if (!abstract_object) {
            log_warning << "Encountered expired Object with handle:" << handle;
            return {};
        }
        std::shared_ptr<T> cast_object = std::dynamic_pointer_cast<T>(abstract_object);
        if (!cast_object) {
            log_warning << "Requested handle " << handle << " as type \"" << typeid(T).name() << "\" "
                        << "but the Object is a \"" << typeid(abstract_object.get()).name() << "\"";
        }
        return cast_object;
    }

private: // methods for AbstractObject
    /// @brief Returns the next, free Handle.
    /// Is thread-safe.
    Handle _get_next_handle();

    /// @brief Registers a new Object with the Manager.
    /// The handle of the Object may not be the BAD_HANDLE, nor may it have been used to register another Object.
    /// @param object   Object to register with its handle.
    /// @return True iff the Object was successfully registered - false if its handle is already taken.
    bool _register_object(std::shared_ptr<Object> object);

    /// @brief Removes the weak_ptr to a given Object when it is deleted.
    /// @param handle   Handle of the Object to release.
    void _release_object(const Handle handle);

private: // fields
    /// @brief The next available handle, is ever-increasing.
    std::atomic<Handle> m_nextHandle;

    /// @brief All Objects in the Application accessible by its unique Handle.
    std::unordered_map<Handle, std::weak_ptr<Object>> m_object;
};

} // namespace notf
