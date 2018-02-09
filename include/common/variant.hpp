#pragma once

#include "common/meta.hpp"

#ifdef NOTF_CPP17

#include <variant>

#else

#include "mpark/variant.hpp"
namespace std {
using namespace mpark;
using mpark::get;
} // namespace std

#endif
