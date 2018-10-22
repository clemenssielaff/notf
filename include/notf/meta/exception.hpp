#pragma once

#include <exception>

#include "./debug.hpp"
#include "./macros.hpp"

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

    // methods --------------------------------------------------------------------------------- //
public:
    NOTF_NO_COPY_OR_ASSIGN(notf_exception);

    /// Constructor with a single (or no) message;
    /// @param file     File containing the function throwing the error.
    /// @param function Function in which the error was thrown.
    /// @param line     Line in `file` at which the error was thrown.
    /// @param message  What has caused this exception (can be empty).
    notf_exception(const char* file, const char* function, const long line, const char* message = "")
        : m_file(filename_from_path(file)), m_function(function), m_line(line), m_message(message)
    {}

    /// Constructor with an fmt formatted message.
    /// @param file     File containing the function throwing the error.
    /// @param function Function in which the error was thrown.
    /// @param line     Line in `file` at which the error was thrown.
    /// @param fmt      Format string used to construct the message.
    /// @param args     Variadic arguments used to fill placeholders in the format string.
    template<class... Args>
    notf_exception(const char* file, const char* function, const long line, const char* fmt, Args&&... args)
        : m_file(filename_from_path(file))
        , m_function(function)
        , m_line(line)
        , m_message(fmt::format(fmt, std::forward<Args>(args)...))
    {}

    /// Destructor.
    ~notf_exception() override = default;

    /// Name of the file in which the exception was thrown.
    const char* get_file() const noexcept { return m_file; }

    /// Name of the function in which the exception was thrown.
    const char* get_function() const noexcept { return m_function; }

    /// Line in the file at which the exception was thrown.
    long get_line() const noexcept { return m_line; }

    /// Returns the explanatory string.
    const char* what() const noexcept override { return m_message.data(); }

    // fields ---------------------------------------------------------------------------------- //
private:
    /// Name of the file in which the exception was thrown.
    const char* m_file;

    /// Name of the function in which the exception was thrown.
    const char* m_function;

    /// Line in the file at which the exception was thrown.
    const long m_line;

    /// The exception message;
    const std::string m_message;
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
    }

/// Specialized exception that logs the message and then behaves like a regular std::runtime_error.
NOTF_EXCEPTION_TYPE(RunTimeError);

/// Exception type for logical errors.
NOTF_EXCEPTION_TYPE(LogicError);

/// Exception type for malformed or otherwise invalid values.
NOTF_EXCEPTION_TYPE(ValueError);

/// Exception type for out of bounds errors.
NOTF_EXCEPTION_TYPE(OutOfBounds);

/// Exception type for access to invalid resources.
NOTF_EXCEPTION_TYPE(ResourceError);

/// Error thrown when something went wrong that really shouldn't have ...
NOTF_EXCEPTION_TYPE(InternalError);

/// Error thrown when the wrong thread does something.
NOTF_EXCEPTION_TYPE(ThreadError);

/// Error thrown when something is not unique as it should be.
NOTF_EXCEPTION_TYPE(NotUniqueError);

NOTF_CLOSE_NAMESPACE
