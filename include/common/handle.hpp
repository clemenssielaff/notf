#pragma once

#include <cstddef>

namespace signal {

using Handle = size_t;

/// \brief Null-value for Handles
static const Handle BAD_HANDLE = 0;

/// \brief Reserved value to identify a root parent Handle.
static const Handle ROOT_HANDLE = 1;

/// \brief The first dynamically assigned Handle value.
static const Handle _FIRST_HANDLE = 1024;

} // namespace signal
