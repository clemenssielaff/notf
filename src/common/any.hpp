#pragma once

#include "common/meta.hpp"

#ifdef NOTF_CPP17
#    include <any>
#else
#    include "abseil/any.hpp"

namespace std {

using absl::any;
using absl::any_cast;
using absl::bad_any_cast;
using absl::make_any;

} // namespace std

#endif
