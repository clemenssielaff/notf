#pragma once

#include "notf/meta/config.hpp"

NOTF_OPEN_NAMESPACE

// test utilities =================================================================================================== //

/// We want 100% test coverage ideally, but some constexpr branches will never be executed on a single machine.
/// For example:
///
///     void do_bit_fiddling
///     {
///         if constexpr (is_big_endian()) { ... }  // either this ...
///         else { ... }                            // ... or this branch
///     }
///
/// Either the first or the second branch can never be executed on the same machine, yet the other is marked as "missed"
/// lines in the code coverage. In order to force the compiler to assemble the function at compile time, we add a bogus
/// template specification to it during the test run. This does not change behavior at all, but causes the unreachable
/// branch not to be emitted in the first place.
#ifdef NOTF_TEST
#define NOTF_MAKE_TEMPLATE_IN_TESTS template<class = void>
#else
#define NOTF_MAKE_TEMPLATE_IN_TESTS
#endif

NOTF_CLOSE_NAMESPACE
