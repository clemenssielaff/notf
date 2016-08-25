#pragma once

#include <memory>
#include <type_traits>

namespace signal {

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
    return static_cast<std::underlying_type_t<Enum>>(enumerator);
}

/// Turns a #define macro into a const char* usable at compile time.
#define XSTRINGIFY(x) #x
#define STRINGIFY(x) XSTRINGIFY(x)

/// Can_copy checks (at compile time) that a T1 can be assigned to a T2.
///
/// See http://www.stroustrup.com/bs_faq2.html#constraints
template <class T1, class T2>
struct CanCopy {
    static void constraints(T1 a, T2 b)
    {
        T2 c = a;
        b = a;
    }
    CanCopy() { void (*p)(T1, T2) = constraints; }
};

// https://stackoverflow.com/a/8147213/3444217 and https://stackoverflow.com/a/25069711/3444217
template <typename T>
class MakeSharedEnabler : public T {
public:
    template <typename... Args>
    MakeSharedEnabler(Args&&... args)
        : T(std::forward<Args>(args)...)
    {
    }
};

} // namespace signal
