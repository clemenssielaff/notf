#pragma once

#include "common/meta.hpp"

#ifdef NOTF_DEBUG

#include "fmt/format.h"

NOTF_OPEN_NAMESPACE

namespace detail {

/// Called when an assertion fails.
/// Prints out a formatted message with information about the failure and then aborts the program.
NOTF_NORETURN inline void assertion_failed(char const* expr, char const* function, char const* file, long line)
{
    using namespace fmt::literals;
    fmt::print(stderr, R"(Assertion "{expr}" failed on "{file}::{line}" in function "{func}"{br})", "expr"_a = expr,
               "file"_a = notf::basename(file), "line"_a = line, "func"_a = function, "br"_a = "\n");
    std::abort();
}

/// Called when an assertion with a message fails.
/// Prints out a formatted message with information about the failure and then aborts the program.
NOTF_NORETURN inline void
assertion_failed_msg(char const* expr, char const* function, char const* file, long line, fmt::MemoryWriter msg)
{
    using namespace fmt::literals;
    fmt::print(stderr, R"(Assertion "{expr}" failed on "{file}::{line}" in function "{func}" with message: {msg}{br})",
               "expr"_a = expr, "file"_a = notf::basename(file), "line"_a = line, "func"_a = function,
               "msg"_a = msg.c_str(), "br"_a = "\n");
    std::abort();
}

/// Creates an in-memory, formatted message to be printed out by `assertion_failed_msg`.
template<typename T, typename... Args>
fmt::MemoryWriter assertion_writer(T&& string, Args&&... args)
{
    fmt::MemoryWriter writer;
    writer.write(std::forward<T>(string), std::forward<Args>(args)...);
    return writer;
}

} // namespace detail

NOTF_CLOSE_NAMESPACE

/// In debug mode, checks an expression and aborts the program with error information on failure.
#define NOTF_ASSERT(expr) \
    (NOTF_LIKELY(expr) ? NOTF_NOOP : ::notf::detail::assertion_failed(#expr, NOTF_CURRENT_FUNCTION, __FILE__, __LINE__))

/// In debug mode, checks an expression and aborts the program with error information and a custom message on failure.
#define NOTF_ASSERT_MSG(expr, ...)                                                                              \
    (NOTF_LIKELY(expr) ? NOTF_NOOP :                                                                            \
                         ::notf::detail::assertion_failed_msg(#expr, NOTF_CURRENT_FUNCTION, __FILE__, __LINE__, \
                                                              ::notf::detail::assertion_writer(__VA_ARGS__)))

#else

/// In release mode, the NoTF assertion macros expand to nothing.
#define NOTF_ASSERT(expr) NOTF_NOOP
#define NOTF_ASSERT_MSG(expr, ...) NOTF_NOOP

#endif
