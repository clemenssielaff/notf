#pragma once

#include <memory>

#include "common/handle.hpp"

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
    explicit Object();

private: // fields
    /** Application-unique Handle. */
    const Handle m_handle;
};

} // namespace notf
