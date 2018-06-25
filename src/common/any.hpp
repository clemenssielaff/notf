#pragma once

#include "common/meta.hpp"

#ifdef NOTF_CPP17

#include <any>
NOTF_OPEN_NAMESPACE
using std::any;
using std::any_cast;
using std::bad_any_cast;
using std::make_any;
NOTF_CLOSE_NAMESPACE

#else

#include "abseil/any.hpp"
NOTF_OPEN_NAMESPACE
using absl::any;
using absl::any_cast;
using absl::bad_any_cast;
using absl::make_any;
NOTF_CLOSE_NAMESPACE

#endif
