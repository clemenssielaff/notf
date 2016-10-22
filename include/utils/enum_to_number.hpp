#pragma once

#include <type_traits>

namespace notf {

/// Const expression to use an enum class value as a numeric value.
///
/// Blatantly copied from "Effective Modern C++ by Scott Mayers': Item #10.
template <typename Enum>
constexpr auto to_number(Enum enumerator) noexcept
{
    return static_cast<std::underlying_type_t<Enum>>(enumerator);
}

} // namespace notf
