#pragma once

#include <cstddef>

namespace notf {

using Handle = size_t;

/// \brief Null-value for Handles
static constexpr Handle BAD_HANDLE = 0;

/// \brief Reserved value to identify a root parent Handle.
static constexpr Handle ROOT_HANDLE = 1;

/// \brief The first dynamically assigned Handle value.
static constexpr Handle _FIRST_HANDLE = 2;

/// \brief Checks if a given Handle is valid or reserved.
inline bool is_valid(const Handle handle) { return handle >= _FIRST_HANDLE; }

} // namespace notf
