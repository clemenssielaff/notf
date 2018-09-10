#pragma once

#include "./numeric.hpp"

NOTF_OPEN_META_NAMESPACE

// string const ======================================================================================================//

/// Compile time string.
struct StringConst {

    template<std::size_t N>
    constexpr StringConst(const char (&a)[N]) noexcept : m_text(a), m_size(N - 1)
    {}

    constexpr char operator[](const std::size_t index) const { return index < m_size ? m_text[index] : throw 0; }

    constexpr std::size_t get_size() const noexcept { return m_size; }

    constexpr const char* c_str() const noexcept { return m_text; }

private:
    const char* const m_text;
    const std::size_t m_size;
};

// string type ====================================================================================================== //

template<char... Cs>
struct StringType final {

    // methods ------------------------------------------------------------------------------------------------------ //
public:
    /// Returns the numbers of letters in the TypeString without the closing null.
    static constexpr std::size_t get_size() noexcept { return sizeof...(Cs); }

    constexpr const char* c_str() const noexcept { return s_text; }

    /// Tests if two TypeStrings are the same..
    template<class Other, typename Indices = std::make_index_sequence<min(get_size(), Other::get_size())>>
    static constexpr bool is_same() noexcept
    {
        if constexpr (get_size() != Other::get_size()) {
            return false;
        }
        return _is_same_impl<Other>(Indices{});
    }
    template<class Other>
    static constexpr bool is_same(Other) noexcept
    {
        return is_same<Other>();
    }

private:
    /// Helper function folding a character comparison over the complete string.
    template<class Other, std::size_t... I>
    static constexpr bool _is_same_impl(std::index_sequence<I...>) noexcept
    {
        return ((s_text[I] == Other::c_str[I]) && ...);
    }

    // members ------------------------------------------------------------------------------------------------------ //
private:
    /// C-string compile-time access to the characters passed as template arguments.
    static inline constexpr char const s_text[sizeof...(Cs) + 1] = {Cs..., '\0'};
};

namespace detail {

template<class T>
auto concat_string_type_impl(T&& last) // recursion breaker
{
    return last;
}
template<char... Ls, char... Rs, class... Tail>
auto concat_string_type_impl(StringType<Ls...>, StringType<Rs...>, Tail... tail)
{
    return concat_string_type_impl(StringType<Ls..., Rs...>{}, std::forward<Tail>(tail)...);
}

template<const StringConst& string, std::size_t... I>
static auto constexpr string_const_to_string_type(std::index_sequence<I...>)
{
    return StringType<string[I]...>{};
}

template<uchar number>
constexpr auto digit_to_string_type()
{
    static_assert(number <= 9);
    return StringType<static_cast<char>(48 + number)>{};
}

template<std::size_t number, std::size_t... I>
static auto constexpr number_to_string_type(std::index_sequence<I...>)
{
    constexpr std::size_t last_index = sizeof...(I) - 1; // reverse the expansion
    return concat_string_type_impl(digit_to_string_type<get_digit(number, last_index - I)>()...);
}

} // namespace detail

/// Creates a new StringType from a StringConst or integral literal.
/// Note that the StringConst value that is passed must have linkage.
/// Example:
///   constexpr StringConst test = "test";
///   using TestType = decltype(make_string_type<test>());
template<const StringConst& string>
constexpr auto make_string_type()
{
    return detail::string_const_to_string_type<string>(std::make_index_sequence<string.get_size()>{});
}
template<std::size_t number>
constexpr auto make_string_type()
{
    return detail::number_to_string_type<number>(std::make_index_sequence<count_digits(number)>{});
}

/// Helper typdef for defining a StringType with a *constexpr* value.
/// Example:
///
///     constexpr StringConst example_string = "example";
///     constexpr size_t example_number = 123;
///     using example_string_t = make_string_type_t<example_string>;    // works
///     using example_number_t = make_string_type_t<example_number>;    // works
///     // using example_string_t = make_string_type_t<"example">;      // does not work
///     // using example_number_t = make_string_type_t<123>;            // does not work
///
template<const auto& arg>
using make_string_type_t = decltype(make_string_type<arg>());

/// Concatenates multiple StringTypes into a single new one.
template<class... Ts>
using concat_string_type = decltype(detail::concat_string_type_impl(Ts{}...));

NOTF_CLOSE_META_NAMESPACE
