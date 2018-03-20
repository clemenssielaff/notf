#pragma once

#include "common/meta.hpp"

#ifdef NOTF_CPP17
#include <optional>
#else
#include "abseil/optional.hpp"

namespace std {

using absl::bad_optional_access;
using absl::make_optional;
using absl::nullopt;
using absl::nullopt_t;
using absl::optional;

} // namespace std

#endif
