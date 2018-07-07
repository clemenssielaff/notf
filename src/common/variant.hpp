#pragma once

#include "common/meta.hpp"

#ifdef NOTF_CPP17

#include <variant>
NOTF_OPEN_NAMESPACE
using std::bad_variant_access;
using std::variant;
using std::variant_size;
using std::variant_alternative;
using std::variant_alternative_t;
using std::get;
using std::holds_alternative;
using std::visit;
NOTF_CLOSE_NAMESPACE

#else

#include "mpark/variant.hpp"
NOTF_OPEN_NAMESPACE
using mpark::bad_variant_access;
using mpark::variant;
using mpark::variant_size;
using mpark::variant_alternative;
using mpark::variant_alternative_t;
using mpark::get;
using mpark::holds_alternative;
using mpark::visit;
NOTF_CLOSE_NAMESPACE

#endif
