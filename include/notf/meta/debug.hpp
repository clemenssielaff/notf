#pragma once

#include <utility>

#include "notf/meta/config.hpp"

NOTF_OPEN_NAMESPACE

// introspection ==================================================================================================== //

/// Useful for debug-switches at compile time.
#ifdef NOTF_DEBUG
constexpr bool is_debug_build() noexcept { return true; }
#else
constexpr bool is_debug_build() noexcept { return false; }
#endif

/// Extracts the last occurrence of a token in a null-terminated string at compile time.
/// In practice, this is used to get the file name from a path read from the __FILE__ macro.
/// @param input     Path to investigate.
/// @return          Only the last part of the path, e.g. basename("/path/to/some/file.cpp", '/') would return
///                  "file.cpp".
template<char DELIMITER = '/'>
constexpr const char* filename_from_path(const char* input)
{
    std::size_t last_occurrence = 0;
    for (std::size_t offset = 0; input[offset]; ++offset) {
        if (input[offset] == DELIMITER) {
            last_occurrence = offset + 1;
        }
    }

    return &input[last_occurrence];
}

// run time behavior ================================================================================================ //

/// Function name macro to use for logging and exceptions.
#ifdef NOTF_CLANG
#define NOTF_CURRENT_FUNCTION __PRETTY_FUNCTION__
#else
#ifdef NOTF_MSVC
#define NOTF_CURRENT_FUNCTION __FUNCTION__
#else
#ifdef NOTF_GCC
#define NOTF_CURRENT_FUNCTION __PRETTY_FUNCTION__
#else
#define NOTF_CURRENT_FUNCTION __func__
#endif
#endif
#endif

NOTF_CLOSE_NAMESPACE
