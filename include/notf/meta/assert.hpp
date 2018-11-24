#pragma once

#include "notf/meta/macros.hpp"

#ifdef NOTF_DEBUG

// is only included in debug builds
#include "notf/meta/exception.hpp"

NOTF_OPEN_NAMESPACE

// assertions ======================================================================================================= //

/// Exception thrown when an assertion failed.
NOTF_EXCEPTION_TYPE(AssertionError);

namespace detail {

/// Called when an assertion with a message fails.
/// Prints out a formatted message with information about the failure and then aborts the program.
template<class... Args>
NOTF_NORETURN void
assertion_failed(char const* expr, char const* file, char const* function, long line, const char* fmt, Args&&... args) {
    using namespace fmt::literals;
    const std::string msg
        = fmt::format(R"(Assertion "{expr}" failed at "{file}:{line}" in function "{func}" with message: "{msg}")",
                      "expr"_a = expr, "file"_a = filename_from_path(file), "line"_a = line, "func"_a = function,
                      "msg"_a = fmt::format(fmt, std::forward<Args>(args)...));
    if constexpr (::notf::config::abort_on_assert()) {
        std::abort();
    } else {
        throw AssertionError(file, function, line, msg.c_str());
    }
}
NOTF_NORETURN inline void
assertion_failed(char const* expr, char const* file, char const* function, long line, const char* message = nullptr) {
    using namespace fmt::literals;
    std::string msg;
    if (static_cast<bool>(message)) {
        msg = fmt::format(R"(Assertion "{expr}" failed at "{file}:{line}" in function "{func}" with message: "{msg}")",
                          "expr"_a = expr, "file"_a = filename_from_path(file), "line"_a = line, "func"_a = function,
                          "msg"_a = message);
    } else {
        msg = fmt::format(R"(Assertion "{expr}" failed at "{file}:{line}" in function "{func}")", "expr"_a = expr,
                          "file"_a = filename_from_path(file), "line"_a = line, "func"_a = function);
    }
    if constexpr (::notf::config::abort_on_assert()) {
        std::abort();
    } else {
        throw AssertionError(file, function, line, msg.c_str());
    }
}

} // namespace detail

/// Assertion macro that, on failure, logs the failed expression and the code location before aborting.
#define NOTF_ASSERT(expr, ...) \
    (NOTF_LIKELY(expr) ?       \
         NOTF_NOOP :           \
         ::notf::detail::assertion_failed(#expr, __FILE__, NOTF_CURRENT_FUNCTION, __LINE__, ##__VA_ARGS__))

/// Assertion macro like NOTF_ASSERT, but executed the expression in release builds as well
#define NOTF_ASSERT_ALWAYS(expr, ...) NOTF_DEFER(NOTF_ASSERT, expr, ##__VA_ARGS__)

NOTF_CLOSE_NAMESPACE

#else

/// In release mode, the notf assertion macro expands to nothing.
#define NOTF_ASSERT(...)

/// The _ALWAYS assertion macro is always executed, but not checked in release mode.
#define NOTF_ASSERT_ALWAYS(expr, ...) NOTF_IGNORE_VARIADIC(expr, , ##__VA_ARGS__)

#endif
