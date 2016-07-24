#pragma once

namespace signal {

#include <type_traits>

/// Macro silencing 'unused parameter / argument' warnings and making it clear that the variable will not be used.
///
/// (although it doesn't stop from from still using it).
#define UNUSED(x) (void)(x);

/// Const expression to use an enum class value as a numeric value.
///
/// Blatantly copied from "Effective Modern C++ by Scott Mayers': Item #10.
template <typename Enum>
constexpr auto to_number(Enum enumerator) noexcept
{
    return static_cast<std::underlying_type_t<Enum> >(enumerator);
}

} // namespace signal
