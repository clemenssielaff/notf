#pragma once

#include "common/meta.hpp"

#ifdef CPP17

#include <string_view>

#else

#include "cpp17_headers/string_view.hpp"
namespace std {
using namespace stx;
} // namespace std

#endif
