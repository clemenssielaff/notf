#pragma once

#include <stdexcept>

#include "common/string.hpp"

namespace notf {

//====================================================================================================================//

/// Specialized Exception thrown when you try to destroy the universe.
class division_by_zero : public std::logic_error {
public: // methods
    /// Default Constructor.
    division_by_zero() : std::logic_error("Division by zero!") {}

    /// Destructor.
    virtual ~division_by_zero() override;
};

//====================================================================================================================//

/// Specialized Exception that logs the message and then behaves like a regular std::runtime_error.
class runtime_error : public std::runtime_error {
public: // methods
    /// Value Constructor.
    /// @param message   What has caused this exception?
    /// @param file      File containing the function throwing the error.
    /// @param function  Function in which the error was thrown.
    /// @param line      Line in `file` at which the error was thrown.
    runtime_error(const std::string& message, std::string file, std::string function, uint line);

    /// Destructor.
    virtual ~runtime_error() override;
};

//====================================================================================================================//

/// Error thrown by risky_ptr, when you try to dereference a nullptr.
class bad_deference_error : public std::runtime_error {
public: // methods
    /// Default Constructor.
    bad_deference_error() : std::runtime_error("Failed to dereference an empty pointer!") {}

    /// Destructor.
    virtual ~bad_deference_error() override;
};

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
            throw bad_deference_error();
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
            throw bad_deference_error();
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

/// Convenience macro to throw a notf::runtime_error that contains the line, file and function where the error
/// occured.
#ifndef throw_runtime_error
#define throw_runtime_error(msg) (throw notf::runtime_error(msg, notf::basename(__FILE__), __FUNCTION__, __LINE__))
#else
#warning "Macro 'throw_runtime_error' is already defined - NoTF's throw_runtime_error macro will remain disabled."
#endif
