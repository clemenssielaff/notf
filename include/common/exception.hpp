#pragma once

#include <stdexcept>

namespace notf {

/** Extracts the last part of a pathname at compile time.
 * @param input     Path to investigate.
 * @param delimiter Delimiter used to separate path elements.
 * @return          Only the last part of the path, e.g. basename("/path/to/some/file.cpp", '/') would return "file.cpp".
 */
constexpr const char* basename(const char* input, const char delimiter = '/')
{
    size_t last_occurrence = 0;
    for (size_t offset = 0; input[offset]; ++offset) {
        if (input[offset] == delimiter) {
            last_occurrence = offset + 1;
        }
    }
    return &input[last_occurrence];
}

/**********************************************************************************************************************/

/** Specialized Exception thrown when you try to destroy the universe. */
class division_by_zero : public std::logic_error {
public: // methods
    /** Default Constructor. */
    division_by_zero()
        : std::logic_error("Division by zero!") {}

    /** Destructor. */
    virtual ~division_by_zero() override;
};

/**********************************************************************************************************************/

/** Specialized Exception that logs the message and then behaves like a regular std::runtime_error. */
class runtime_error : public std::runtime_error {
public: // methods
    /** Value Constructor.
     * @param message   What has caused this exception?
     * @param file      File containing the function throwing the error.
     * @param function  Function in which the error was thrown.
     * @param line      Line in `file` at which the error was thrown.
     */
    runtime_error(const std::string& message, std::string file, std::string function, uint line);

    /** Destructor. */
    virtual ~runtime_error() override;
};

} // namespace notf

/** Convenience macro to throw a notf::runtime_error that contains the line, file and function where the error occured. */
#ifndef throw_runtime_error
#define throw_runtime_error(msg) (throw notf::runtime_error(msg, notf::basename(__FILE__), __FUNCTION__, __LINE__))
#else
#warning "Macro 'throw_runtime_error' is already defined - NoTF's throw_runtime_error macro will remain disabled."
#endif