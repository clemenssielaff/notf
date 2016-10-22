#pragma once

#include <atomic>
#include <memory>
#include <string>
#include <unordered_map>

#include "common/handle.hpp"

namespace notf {

class AbstractObject;

/*
 * \brief Mangager of everything in an Application that is accessible through its unique Handle.
 */
class ObjectManager {

    friend class AbstractObject;

public: // methods
    /// \brief Default Constructor.
    /// \param reserve  How many Objects to reserve space for initially.
    explicit ObjectManager(const size_t reserve = 1024);

    /// \brief Checks if the the given Handle denotes a known Object.
    bool has_object(const Handle handle) const { return m_object.count(handle); }

    /// \brief Returns an AbsractObject by its Handle.
    /// \param handle   Handle associated with the requested Object.
    /// \return Shared pointer to the Object, is invalid on error.
    std::shared_ptr<AbstractObject> get_object(const Handle handle) const { return get_abstract_object(handle); }

    /// \brief Returns an Object by its Handle.
    /// The returned pointer is invalid if the handle does not identify an Object or the Object is of the wrong type.
    /// \param handle   Handle associated with the requested Object.
    /// \return Shared pointer to the Object, is invalid on error.
    template <typename T, typename = typename std::enable_if<std::is_base_of<AbstractObject, T>::value>::type>
    std::shared_ptr<T> get_object(const Handle handle) const
    {
        std::shared_ptr<AbstractObject> abstract_object = get_abstract_object(handle);
        if (!abstract_object) {
            return {};
        }
        std::shared_ptr<T> cast_object = std::dynamic_pointer_cast<T>(abstract_object);
        if (!cast_object) {
            wrong_type_warning(typeid(T).name(), handle);
        }
        return cast_object;
    }

private: // methods for AbstractObject
    /// \brief Returns the next, free Handle.
    /// Is thread-safe.
    Handle get_next_handle();

    /// \brief Registers a new Object with the Manager.
    /// The handle of the Object may not be the BAD_HANDLE, nor may it have been used to register another Object.
    /// \param object   Object to register with its handle.
    /// \return True iff the Object was successfully registered - false if its handle is already taken.
    bool register_object(std::shared_ptr<AbstractObject> object);

    /// \brief Removes the weak_ptr to a given Object when it is deleted.
    /// \param handle   Handle of the Object to release.
    void release_object(const Handle handle);

private: // methods
    /// \brief Accessor method providing the opportunity for logging.
    std::shared_ptr<AbstractObject> get_abstract_object(const Handle handle) const;

    /// \brief Wrapper for logging.
    void wrong_type_warning(const std::string& type_name, const Handle handle) const;

private: // fields
    /// \brief The next available handle, is ever-increasing.
    std::atomic<Handle> m_nextHandle;

    /// \brief All Objects in the Application accessible by its unique Handle.
    std::unordered_map<Handle, std::weak_ptr<AbstractObject>> m_object;
};

} // namespace notf
