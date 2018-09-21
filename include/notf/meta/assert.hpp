#pragma once

#ifdef NOTF_DEBUG

#include "./log.hpp" // is only included in debug builds

NOTF_OPEN_NAMESPACE

// assertions ======================================================================================================= //

namespace detail {

/// Called when an assertion with a message fails.
/// Prints out a formatted message with information about the failure and then aborts the program.
template<class... Args>
[[noreturn]] void
assertion_failed(char const* expr, char const* function, char const* file, long line, const char* fmt, Args&&... args)
{
    using namespace fmt::literals;
    TheLogger::get()->critical(                                                                       //
        R"(Assertion "{expr}" failed at "{file}:{line}" in function "{func}" with message: "{msg}")", //
        "expr"_a = expr, "file"_a = filename_from_path(file), "line"_a = line, "func"_a = function,
        "msg"_a = fmt::format(fmt, std::forward<Args>(args)...));
    std::abort();
}
[[noreturn]] inline void
assertion_failed(char const* expr, char const* function, char const* file, long line, const char* message = nullptr)
{
    using namespace fmt::literals;
    if (message) {
        TheLogger::get()->critical(                                                                       //
            R"(Assertion "{expr}" failed at "{file}:{line}" in function "{func}" with message: "{msg}")", //
            "expr"_a = expr, "file"_a = filename_from_path(file), "line"_a = line, "func"_a = function,
            "msg"_a = message);
    }
    else {
        TheLogger::get()->critical(                                                 //
            R"(Assertion "{expr}" failed at "{file}:{line}" in function "{func}")", //
            "expr"_a = expr, "file"_a = filename_from_path(file), "line"_a = line, "func"_a = function);
    }
    std::abort();
}

} // namespace detail

/// Assertion macro that, on failure, logs the failed expression and the code location before aborting.
#define NOTF_ASSERT(expr, ...) \
    (NOTF_LIKELY(expr) ?       \
         NOTF_NOOP :           \
         ::notf::detail::assertion_failed(#expr, NOTF_CURRENT_FUNCTION, __FILE__, __LINE__, ##__VA_ARGS__))

NOTF_CLOSE_NAMESPACE

#else

/// In release mode, the notf assertion macro expands to nothing.
#define NOTF_ASSERT(...)

#endif
