#pragma once

#include "./meta.hpp"

NOTF_OPEN_META_NAMESPACE

// introspection ==================================================================================================== //

/// Useful for debug-switches at compile time.
#ifdef NOTF_DEBUG
constexpr bool is_debug_build() noexcept { return true; }
#else
constexpr bool is_debug_build() noexcept { return false; }
#endif

// run time behavior ================================================================================================ //

/// Function name macro to use for logging and exceptions.
#ifdef NOTF_LOG_PRETTY_FUNCTION
#ifdef NOTF_CLANG
#define NOTF_CURRENT_FUNCTION __PRETTY_FUNCTION__
#else
#ifdef NOTF_MSVC
#define NOTF_CURRENT_FUNCTION __FUNCTION__
#else
#ifdef NOTF_GCC
#define NOTF_CURRENT_FUNCTION __PRETTY_FUNCTION__
#endif
#endif
#endif
#else
#define NOTF_CURRENT_FUNCTION __func__
#endif

NOTF_CLOSE_META_NAMESPACE
