#pragma once

#include "common/meta.hpp"

#ifdef NOTF_CPP17
#include <optional>

NOTF_OPEN_NAMESPACE
using std::bad_optional_access;
using std::make_optional;
using std::nullopt;
using std::nullopt_t;
using std::optional;
NOTF_CLOSE_NAMESPACE

#else
#include "abseil/optional.hpp"

NOTF_OPEN_NAMESPACE
using absl::bad_optional_access;
using absl::make_optional;
using absl::nullopt;
using absl::nullopt_t;
using absl::optional;
NOTF_CLOSE_NAMESPACE

#endif
