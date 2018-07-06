#pragma once

#include <exception>

#include "common/meta.hpp"

#include "fmt/format.h"
#include "fmt/ostream.h"

NOTF_OPEN_NAMESPACE

// ================================================================================================================== //

/// NoTF base exception type.
/// Logs the message with log level `critical`.
struct notf_exception : public std::exception {
    /// Value Constructor.
    /// @param file      File containing the function throwing the error.
    /// @param function  Function in which the error was thrown.
    /// @param line      Line in `file` at which the error was thrown.
    /// @param message   What has caused this exception?
    notf_exception(std::string file, std::string function, uint line, std::string msg = {});

    /// Destructor.
    virtual ~notf_exception() override;

    /// Returns the explanatory string.
    virtual const char* what() const noexcept override { return message.data(); }

    /// The exception message;
    const std::string message;
};

// ================================================================================================================== //

/// Declare a new NoTF exception type.
/// Note that this declares but does not implement the virtual destructor, in order to avoid warnings about emitting the
/// class' v-table into every unit, you'll have to implement the (empty) destructor somewhere manually.
#ifndef NOTF_EXCEPTION_TYPE
#define NOTF_EXCEPTION_TYPE(TYPE)                                                              \
    struct TYPE : public notf::notf_exception {                                                \
        TYPE(std::string file, std::string function, uint line, std::string msg)               \
            : notf::notf_exception(std::move(file), std::move(function), line, std::move(msg)) \
        {}                                                                                     \
        TYPE(const TYPE&) = default;                                                           \
        virtual ~TYPE() override;                                                              \
    }
#else
NOTF_COMPILER_WARNING(
    "Macro 'NOTF_EXCEPTION_TYPE' is already defined - NoTF's NOTF_EXCEPTION_TYPE macro will remain disabled.")
#endif

// ================================================================================================================== //

/// Convenience macro to throw a notf_exception with a message, that additionally contains the line, file and function
/// where the error occured.
#ifndef NOTF_THROW
#define NOTF_THROW(TYPE, ...)                                                                            \
    {                                                                                                    \
        throw TYPE(notf::basename(__FILE__), NOTF_CURRENT_FUNCTION, __LINE__, fmt::format(__VA_ARGS__)); \
    }
#else
NOTF_COMPILER_WARNING("Macro 'NOTF_THROW' is already defined - NoTF's NOTF_THROW macro will remain disabled.")
#endif

// ================================================================================================================== //

/// Specialized exception that logs the message and then behaves like a regular std::runtime_error.
NOTF_EXCEPTION_TYPE(runtime_error);

/// Exception type for logical errors.
NOTF_EXCEPTION_TYPE(logic_error);

/// Exception type for malformed or otherwise invalid values.
NOTF_EXCEPTION_TYPE(value_error);

/// Exception type for out of bounds errors.
NOTF_EXCEPTION_TYPE(out_of_bounds);

/// Exception type for access to invalid resources.
NOTF_EXCEPTION_TYPE(resource_error);

/// Error thrown when something went wrong that really shouldn't have ...
NOTF_EXCEPTION_TYPE(internal_error);

/// Error thrown when the wrong thread does something.
NOTF_EXCEPTION_TYPE(thread_error);

// ================================================================================================================== //

/// Save narrowing cast.
/// https://github.com/Microsoft/GSL/blob/master/include/gsl/gsl_util
template<class TARGET, class RAW_SOURCE>
inline constexpr TARGET narrow_cast(RAW_SOURCE&& value)
{
    using SOURCE = std::decay_t<RAW_SOURCE>;

    TARGET result = static_cast<TARGET>(std::forward<RAW_SOURCE>(value));
    if (static_cast<SOURCE>(result) != value) {
        NOTF_THROW(logic_error, "narrow_cast failed");
    }

    if (!is_same_signedness<TARGET, SOURCE>::value) {
        if ((result < TARGET{}) != (value < SOURCE{})) {
            NOTF_THROW(logic_error, "narrow_cast failed");
        }
    }

    return result;
}

NOTF_CLOSE_NAMESPACE
