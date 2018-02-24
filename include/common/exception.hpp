#pragma once

#include <exception>

#include "common/meta.hpp"
#include "common/string.hpp"

namespace notf {

//====================================================================================================================//

/// NoTF base exception type.
/// Logs the message with log level `critical`.
struct notf_exception : public std::exception {
    /// Value Constructor.
    /// @param file      File containing the function throwing the error.
    /// @param function  Function in which the error was thrown.
    /// @param line      Line in `file` at which the error was thrown.
    /// @param message   What has caused this exception?
    notf_exception(std::string file, std::string function, uint line, std::string message = {});

    /// Destructor.
    virtual ~notf_exception() override;

    /// Returns the explanatory string.
    virtual const char* what() const noexcept override { return message.data(); }

    /// The exception message;
    const std::string message;
};

//====================================================================================================================//

/// Declare a new NoTF exception type.
/// Note that this declares but does not implement the virtual destructor, in order to avoid warnings about emitting the
/// class' v-table into every unit, you'll have to implement the (empty) destructor somewhere manually.
#ifndef NOTF_EXCEPTION_TYPE
#define NOTF_EXCEPTION_TYPE(TYPE)                                                                  \
    struct TYPE : public notf::notf_exception {                                                    \
        TYPE(std::string file, std::string function, uint line, std::string message)               \
            : notf::notf_exception(std::move(file), std::move(function), line, std::move(message)) \
        {}                                                                                         \
        TYPE(const TYPE&) = default;                                                               \
        virtual ~TYPE() override;                                                                  \
    };
#else
#warning "Macro 'NOTF_EXCEPTION_TYPE' is already defined - NoTF's NOTF_EXCEPTION_TYPE macro will remain disabled."
#endif

//====================================================================================================================//

/// Convenience macro to throw a notf_exception with a message, that additionally contains the line, file and function
/// where the error occured.
#ifndef notf_throw_format
#define notf_throw_format(TYPE, MSG)                                             \
    {                                                                            \
        std::stringstream ss;                                                    \
        ss << MSG;                                                               \
        throw TYPE(notf::basename(__FILE__), NOTF_FUNCTION, __LINE__, ss.str()); \
    }
#else
#warning "Macro 'notf_throw_format' is already defined - NoTF's notf_throw_format macro will remain disabled."
#endif

//====================================================================================================================//

/// Convenience macro to trow a notf_exception with a message from a constexpr function.
#ifndef notf_throw
#define notf_throw(TYPE, MSG) (throw TYPE(notf::basename(__FILE__), NOTF_FUNCTION, __LINE__, MSG))
#else
#warning "Macro 'notf_throw' is already defined -"
" NoTF's notf_throw macro will remain disabled."
#endif

//====================================================================================================================//

/// Specialized exception that logs the message and then behaves like a regular std::runtime_error.
NOTF_EXCEPTION_TYPE(runtime_error)

/// Exception type for logical (math) errors.
NOTF_EXCEPTION_TYPE(logic_error)

/// Exception type for out of bounds errors.
NOTF_EXCEPTION_TYPE(out_of_range)

/// Exception type for access to invalid resources.
NOTF_EXCEPTION_TYPE(resource_error)

/// Error thrown when something went wrong that really shouldn't have ...
NOTF_EXCEPTION_TYPE(internal_error)

/// Error thrown by risky_ptr, when you try to dereference a nullptr.
NOTF_EXCEPTION_TYPE(bad_deference_error)

//====================================================================================================================//

/// Pointer wrapper to make sure that if a function can return a nullptr, the user either checks it before dereferencing
/// or doesn't use it.
template<typename T>
struct risky_ptr {

    /// Constructor
    /// @param ptr  Pointer to wrap.
    risky_ptr(T* ptr) : raw(ptr) {}

    /// @{
    /// Access the value pointed to by the wrapped pointer.
    /// @throws bad_deference_error If the wrapped pointer is empty.
    T* operator->()
    {
        if (!raw) {
            notf_throw(bad_deference_error, "Failed to dereference an empty pointer!");
        }
        return raw;
    }
    const T* operator->() const { return const_cast<risky_ptr<T>*>(this); }
    /// @}

    /// Dereferences the wrapped pointer.
    /// @throws bad_deference_error If the wrapped pointer is empty.
    T& operator*()
    {
        if (!raw) {
            notf_throw(bad_deference_error, "Failed to dereference an empty pointer!");
        }
        return *raw;
    }
    const T& operator*() const { return *const_cast<risky_ptr<T>*>(this); }
    /// @}

    /// @{
    /// Equality operator
    bool operator==(const T* rhs) const noexcept { return raw == rhs; }
    bool operator==(const risky_ptr* rhs) const noexcept { return raw == rhs->raw; }
    /// @}

    /// @{
    /// Inequality operator.
    bool operator!=(const T* rhs) const noexcept { return raw != rhs; }
    bool operator!=(const risky_ptr* rhs) const noexcept { return raw != rhs->raw; }
    /// @}

    /// Tests if the contained pointer is save.
    explicit operator bool() const noexcept { return raw != nullptr; }

    /// Tests if the contained pointer is empty.
    bool operator!() const noexcept { return raw == nullptr; }

    /// Tests if the contained pointer is save.
    bool is_save() const noexcept { return raw != nullptr; }

    /// Tests if the contained pointer is empty.
    bool is_empty() const noexcept { return raw == nullptr; }

    /// Wrapped pointer.
    T* const raw;
};

//====================================================================================================================//

} // namespace notf
