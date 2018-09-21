#pragma once

#include <exception>

#include "./debug.hpp"

#include "fmt/format.h"

NOTF_OPEN_NAMESPACE

// notf_exception =================================================================================================== //

/// NoTF base exception type.
/// The tempalte argument is only to distinguish different exception types.
/// Define a new exception type like this:
///
///     struct MyExceptionType{};
///     using MyException = notf_exception<MyExceptionType>;
///     throw MyException(__FILE__, NOTF_CURRENT_FUNCTION, __LINE__, "message");
///
struct notf_exception : public std::exception {

    // methods ------------------------------------------------------------------------------------------------------ //
public:
    /// Constructor with a single (or no) message;
    /// @param file     File containing the function throwing the error.
    /// @param function Function in which the error was thrown.
    /// @param line     Line in `file` at which the error was thrown.
    /// @param message  What has caused this exception (can be empty).
    notf_exception(const char* file, const char* function, const size_t line, const char* message = "")
        : file(filename_from_path(file)), function(function), line(line), message(message)
    {}

    /// Constructor with an fmt formatted message.
    /// @param file     File containing the function throwing the error.
    /// @param function Function in which the error was thrown.
    /// @param line     Line in `file` at which the error was thrown.
    /// @param fmt      Format string used to construct the message.
    /// @param args     Variadic arguments used to fill placeholders in the format string.
    template<class... Args>
    notf_exception(const char* file, const char* function, const size_t line, const char* fmt, Args&&... args)
        : file(filename_from_path(file))
        , function(function)
        , line(line)
        , message(fmt::format(fmt, std::forward<Args>(args)...))
    {}

    /// Destructor.
    ~notf_exception() override = default;

    /// Returns the explanatory string.
    const char* what() const noexcept override { return message.data(); }

    // fields ------------------------------------------------------------------------------------------------------- //
public:
    /// Name of the file in which the exception was thrown.
    const char* file;

    /// Name of the function in which the exception was thrown.
    const char* function;

    /// Line in the file at which the exception was thrown.
    const size_t line;

    /// The exception message;
    const std::string message;
};

/// Convenience macro to throw a notf_exception with a message, that additionally contains the line, file and function
/// where the error occured.
#define NOTF_THROW(TYPE, ...) throw TYPE(__FILE__, NOTF_CURRENT_FUNCTION, __LINE__, ##__VA_ARGS__)

// built-in exception types ========================================================================================= //

/// Declare a new NoTF exception type.
#define NOTF_EXCEPTION_TYPE(TYPE)                                          \
    struct TYPE : public ::notf::notf_exception {                          \
        template<class... Ts>                                              \
        TYPE(Ts&&... ts) : ::notf::notf_exception(std::forward<Ts>(ts)...) \
        {}                                                                 \
    };

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

NOTF_CLOSE_NAMESPACE
