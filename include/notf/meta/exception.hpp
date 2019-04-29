#pragma once

#include <exception>

#include "notf/meta/debug.hpp"
#include "notf/meta/macros.hpp"
#include "notf/meta/typename.hpp"

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
    /// @param type     Type of the error.
    /// @param message  What has caused this exception (can be empty).
    notf_exception(const char* file, const char* function, const long line, const char* type, const char* message = "")
        : m_file(filename_from_path(file)), m_function(function), m_line(line), m_type(type), m_message(message) {}

    /// Constructor with an fmt formatted message.
    /// @param file     File containing the function throwing the error.
    /// @param function Function in which the error was thrown.
    /// @param line     Line in `file` at which the error was thrown.
    /// @param type     Type of the error.
    /// @param fmt      Format string used to construct the message.
    /// @param args     Variadic arguments used to fill placeholders in the format string.
    template<class... Args>
    notf_exception(const char* file, const char* function, const long line, const char* type, const char* fmt,
                   Args&&... args)
        : m_file(filename_from_path(file))
        , m_function(function)
        , m_line(line)
        , m_type(type)
        , m_message(fmt::format(fmt, std::forward<Args>(args)...)) {}

    /// Destructor.
    virtual ~notf_exception() override = default;

    /// Name of the file in which the exception was thrown.
    const char* get_file() const noexcept { return m_file; }

    /// Name of the function in which the exception was thrown.
    const char* get_function() const noexcept { return m_function; }

    /// Type of the error.
    const char* get_type() const noexcept { return m_type; }

    /// Line in the file at which the exception was thrown.
    long get_line() const noexcept { return m_line; }

    /// The raw error message, without information about the error's location.
    const std::string& get_message() const noexcept { return m_message; }

    /// Returns the explanatory string.
    const char* what() const noexcept override {
        if (m_formatted_message.empty()) {
            m_formatted_message = fmt::format("[{}] {} ({}:{})", m_type, m_message, m_file, m_line);
        }
        return m_formatted_message.data();
    }

    // fields ---------------------------------------------------------------------------------- //
private:
    /// Name of the file in which the exception was thrown.
    const char* m_file;

    /// Name of the function in which the exception was thrown.
    const char* m_function;

    /// Line in the file at which the exception was thrown.
    const long m_line;

    /// Type of the error.
    const char* m_type;

    /// The exception message;
    const std::string m_message;

    /// The formatted message, is needed because we need to return a pointer to char from `what` and we need to make
    /// sure that the memory stays valid after the function has returned.
    mutable std::string m_formatted_message;
};

/// Convenience macro to throw a notf_exception with a message, that additionally contains the line, file and function
/// where the error occured.
#define NOTF_THROW(TYPE, ...) throw TYPE(__FILE__, NOTF_CURRENT_FUNCTION, __LINE__, #TYPE, ##__VA_ARGS__)

// built-in exception types ========================================================================================= //

/// Declare a new NoTF exception type.
#define NOTF_EXCEPTION_TYPE(TYPE)                                             \
    struct TYPE : public ::notf::notf_exception {                             \
        template<class... Ts>                                                 \
        TYPE(Ts&&... ts) : ::notf::notf_exception(std::forward<Ts>(ts)...) {} \
    }

/// Specialized exception that logs the message and then behaves like a regular std::runtime_error.
NOTF_EXCEPTION_TYPE(RunTimeError);

/// Exception type for logical errors.
NOTF_EXCEPTION_TYPE(LogicError);

/// Exception type for malformed or otherwise invalid values.
NOTF_EXCEPTION_TYPE(ValueError);

/// Exception type for out of index errors.
NOTF_EXCEPTION_TYPE(IndexError);

/// Exception type for access to invalid resources.
NOTF_EXCEPTION_TYPE(ResourceError);

/// Error thrown when something went wrong that really shouldn't have ...
NOTF_EXCEPTION_TYPE(InternalError);

/// Error thrown when the wrong thread does something.
NOTF_EXCEPTION_TYPE(ThreadError);

/// Error thrown when something is not unique as it should be.
NOTF_EXCEPTION_TYPE(NotUniqueError);

/// Error thrown when a name does not match anything.
NOTF_EXCEPTION_TYPE(NameError);

/// Error thrown a type doesn't match.
NOTF_EXCEPTION_TYPE(TypeError);

/// Some kind of input wasn't expected.
NOTF_EXCEPTION_TYPE(InputError);

NOTF_CLOSE_NAMESPACE
