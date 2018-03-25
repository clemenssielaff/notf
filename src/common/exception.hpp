#pragma once

#include <exception>
#ifdef NOTF_DEBUG
#include <sstream>
#endif

#include "common/string.hpp"

NOTF_OPEN_NAMESPACE

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

/// Exception type for logical errors.
NOTF_EXCEPTION_TYPE(logic_error)

/// Exception type for out of bounds errors.
NOTF_EXCEPTION_TYPE(out_of_bounds)

/// Exception type for access to invalid resources.
NOTF_EXCEPTION_TYPE(resource_error)

/// Error thrown when something went wrong that really shouldn't have ...
NOTF_EXCEPTION_TYPE(internal_error)

/// Error thrown by risky_ptr, when you try to dereference a nullptr.
NOTF_EXCEPTION_TYPE(bad_deference_error)

/// Error thrown in debug mode when a NOTF_ASSERT fails.
NOTF_EXCEPTION_TYPE(assertion_error)

/// Error thrown when the wrong thread does something.
NOTF_EXCEPTION_TYPE(thread_error)

//====================================================================================================================//

#ifdef NOTF_DEBUG

/// Pointer wrapper to make sure that if a function can return a nullptr, the user either checks it before dereferencing
/// or doesn't use it.
template<typename T>
struct risky_ptr {

    /// Default constructor.
    risky_ptr() : m_raw(nullptr) {}

    /// Constructor
    /// @param ptr  Pointer to wrap.
    risky_ptr(T* ptr) : m_raw(ptr) {}

    /// Assignment operator.
    risky_ptr& operator=(T* ptr) { m_raw = ptr; }

    /// Allows implicit conversion between compatible risky pointer types.
    template<typename OTHER, typename = std::enable_if_t<std::is_convertible<T*, OTHER*>::value>>
    operator risky_ptr<OTHER>() const
    {
        return risky_ptr<OTHER>(static_cast<OTHER*>(m_raw));
    }

    /// @{
    /// Access the value pointed to by the wrapped pointer.
    /// @throws bad_deference_error If the wrapped pointer is empty.
    T* operator->()
    {
        if (!m_raw) {
            notf_throw(bad_deference_error, "Failed to dereference an empty pointer!");
        }
        return m_raw;
    }
    const T* operator->() const { return const_cast<risky_ptr<T>*>(this)->operator->(); }
    /// @}

    /// Dereferences the wrapped pointer.
    /// @throws bad_deference_error If the wrapped pointer is empty.
    T& operator*()
    {
        if (!m_raw) {
            notf_throw(bad_deference_error, "Failed to dereference an empty pointer!");
        }
        return *m_raw;
    }
    const T& operator*() const { return const_cast<risky_ptr<T>*>(this)->operator*(); }
    /// @}

    /// @{
    /// Equality operator
    bool operator==(const T* rhs) const noexcept { return m_raw == rhs; }
    bool operator==(const risky_ptr& rhs) const noexcept { return m_raw == rhs.m_raw; }
    /// @}

    /// @{
    /// Inequality operator.
    bool operator!=(const T* rhs) const noexcept { return m_raw != rhs; }
    bool operator!=(const risky_ptr& rhs) const noexcept { return m_raw != rhs.m_raw; }
    /// @}

    /// Tests if the contained pointer is save.
    explicit operator bool() const noexcept { return m_raw != nullptr; }

    /// Tests if the contained pointer is empty.
    bool operator!() const noexcept { return m_raw == nullptr; }

    // fields --------------------------------------------------------------------------------------------------------//
private:
    /// Wrapped pointer.
    T* m_raw;

    // friends -------------------------------------------------------------------------------------------------------//
    template<typename U>
    friend U* make_raw(const risky_ptr<U>&) noexcept;

    template<typename U>
    friend const U* make_raw(const risky_ptr<const U>&) noexcept;
};

/// @{
/// Unwraps the a raw pointer from a risky_ptr.
/// @throws bad_deference_error If the wrapped pointer is empty.
template<typename U>
inline U* make_raw(const risky_ptr<U>& risky) noexcept
{
    return risky.m_raw;
}
template<typename U>
inline const U* make_raw(const risky_ptr<const U>& risky) noexcept
{
    return risky.m_raw;
}
/// @}

#else

    /// In release-mode the risky_ptr is a simple typedef that is compiled away to a raw pointer.
    template<typename T>
    using risky_ptr = T*;

template<typename T>
inline T* make_raw(risky_ptr<T>& risky) noexcept
{
    return risky;
}
template<typename T>
inline const T* make_raw(const risky_ptr<T>& risky) noexcept
{
    return risky;
}

#endif

//====================================================================================================================//

#ifdef NOTF_DEBUG

/// NoTF assertion macro.
#define NOTF_ASSERT(EXPR)                                                                          \
    if (!static_cast<bool>(EXPR)) {                                                                \
        notf_throw_format(assertion_error, "Assertion \"" << NOTF_DEFER(NOTF_STR, EXPR) << "\" failed!"); \
    }

/// NoTF assertion macro with attached message.
#define NOTF_ASSERT_MSG(EXPR, MSG)                                                                                 \
    if (!static_cast<bool>(EXPR)) {                                                                                \
        notf_throw_format(assertion_error, "Assertion \"" << NOTF_DEFER(NOTF_STR, EXPR) << "\" failed! Message:" << MSG); \
    }

#else

/// In release mode, the NoTF assertion macros expand to nothing.
#define NOTF_ASSERT(EXPR)          /* assertion */
#define NOTF_ASSERT_MSG(EXPR, MSG) /* assertion */

#endif

//====================================================================================================================//

/// Save narrowing cast.
/// https://github.com/Microsoft/GSL/blob/master/include/gsl/gsl_util
template<class TARGET, class RAW_SOURCE>
inline constexpr TARGET narrow_cast(RAW_SOURCE&& value)
{
    using SOURCE = std::decay_t<RAW_SOURCE>;

    TARGET result = static_cast<TARGET>(std::forward<RAW_SOURCE>(value));
    if (static_cast<SOURCE>(result) != value) {
        notf_throw(logic_error, "narrow_cast failed");
    }

#ifdef NOTF_CPP17
    if constexpr (!is_same_signedness<TARGET, SOURCE>::value) {
#else
    {
#endif
        if ((result < TARGET{}) != (value < SOURCE{})) {
            notf_throw(logic_error, "narrow_cast failed");
        }
    }

    return result;
}

NOTF_CLOSE_NAMESPACE
