#pragma once

#include "./hash.hpp"

NOTF_OPEN_META_NAMESPACE

// string type ====================================================================================================== //

/// Type defined by a string literal at compile time.
template<class char_t, char_t... Cs>
struct StringType final {

    template<class char_t2, char_t2... Os>
    friend struct StringType; // to allow other StringTypes to access the private `s_text` member field.

    // methods ------------------------------------------------------------------------------------------------------ //
public:
    /// The string literal that defines this type.
    static constexpr const char_t* c_str() noexcept { return s_text; }

    /// The numbers of letters in the TypeString without the closing null.
    static constexpr size_t get_size() noexcept { return sizeof...(Cs); }

    /// The hash of the string.
    static constexpr size_t get_hash() noexcept { return hash_string(s_text, get_size()); }

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

    /// Access to an individual character of the string.
    static constexpr char_t at(const size_t index)
    {
        if (index < get_size()) {
            return s_text[index];
        }
        else {
            throw std::out_of_range("Failed to read out-of-range StringType character");
        }
    }

private:
    /// Helper function folding a character comparison over the complete string.
    template<class Other, size_t... I>
    static constexpr bool _is_same_impl(std::index_sequence<I...>) noexcept
    {
        return ((s_text[I] == Other::s_text[I]) && ...);
    }

    // members ------------------------------------------------------------------------------------------------------ //
private:
    /// C-string compile-time access to the characters passed as template arguments.
    inline static constexpr char_t const s_text[sizeof...(Cs) + 1] = {Cs..., '\0'};
};

// string const ======================================================================================================//

namespace detail {

/// Compile time string.
template<class char_t>
struct StringConst {

    // methods ------------------------------------------------------------------------------------------------------ //
public:
    /// Constructor
    /// Takes a compile time literal.
    template<size_t N>
    constexpr StringConst(const char_t (&a)[N]) noexcept : m_text(a), m_size(N - 1)
    {}

    /// The constant string literal.
    constexpr const char_t* c_str() const noexcept { return m_text; }

    /// The numbers of letters in the string without the closing null.
    constexpr size_t get_size() const noexcept { return m_size; }

    /// The hash of the string.
    constexpr size_t get_hash() const noexcept { return hash_string(m_text, m_size); }

    /// operator ==
    constexpr bool operator==(const StringConst& other) const noexcept
    {
        if (other.get_size() != get_size()) {
            return false;
        }
        for (size_t i = 0; i < get_size(); ++i) {
            if (other[i] != m_text[i]) {
                return false;
            }
        }
        return true;
    }

    /// Access to an individual character of the string.
    constexpr char_t operator[](const size_t index) const
    {
        return (index < m_size) ? m_text[index] :
                                  throw std::out_of_range("Failed to read out-of-range StringConst character");
    }

    // members ------------------------------------------------------------------------------------------------------ //
private:
    /// Pointer to the text stored somewhere with linkage.
    const char_t* const m_text;

    /// Numbers of letters in the string without the closing null.
    const size_t m_size;
};

} // namespace detail

/// The default StringConst just takes a array of regular `char`.
using StringConst = detail::StringConst<char>;

// ================================================================================================================== //

/// Comparison between a StringConst and a StringType.
template<class char_t, char_t... Cs>
constexpr bool operator==(const detail::StringConst<char_t>& lhs, const StringType<char_t, Cs...>& rhs) noexcept
{
    if (sizeof...(Cs) != lhs.get_size()) {
        return false;
    }
    for (size_t i = 0; i < lhs.get_size(); ++i) {
        if (rhs.at(i) != lhs[i]) {
            return false;
        }
    }
    return true;
}
template<class char_t, char_t... Cs>
constexpr bool operator==(const StringType<char_t, Cs...>& lhs, const detail::StringConst<char_t>& rhs) noexcept
{
    return rhs == lhs;
}

// ================================================================================================================== //

namespace detail {

template<class T>
auto concat_string_type_impl(T&& last) // recursion breaker
{
    return last;
}
template<class char_t, char_t... Ls, char_t... Rs, class... Tail>
auto concat_string_type_impl(StringType<char_t, Ls...>, StringType<char_t, Rs...>, Tail... tail)
{
    return concat_string_type_impl(StringType<char_t, Ls..., Rs...>{}, std::forward<Tail>(tail)...);
}

template<class char_t, const StringConst<char_t>& string, size_t... I>
[[maybe_unused]] static auto constexpr string_const_to_string_type(std::index_sequence<I...>)
{
    return StringType<char_t, string[I]...>{};
}

template<class char_t, char_t number>
constexpr auto digit_to_string_type()
{
    static_assert(number <= 9);
    return StringType<char_t, static_cast<char_t>(48 + number)>{};
}

template<size_t number, size_t... I>
[[maybe_unused]] static auto constexpr number_to_string_type(std::index_sequence<I...>)
{
    constexpr size_t last_index = sizeof...(I) - 1; // reverse the expansion
    return concat_string_type_impl(digit_to_string_type<get_digit(number, last_index - I)>()...);
}

} // namespace detail

/// Creates a new StringType from a StringConst or integral literal.
/// Note that the StringConst value that is passed must have linkage.
/// Example:
///   constexpr StringConst test = "test";
///   using TestType = decltype(make_string_type<test>());
template<class char_t, const detail::StringConst<char_t>& string>
constexpr auto make_string_type()
{
    return detail::string_const_to_string_type<string>(std::make_index_sequence<string.get_size()>{});
}
template<size_t number>
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

// ================================================================================================================== //

NOTF_OPEN_LITERALS_NAMESPACE

/// User defined literal, turning any string literal into a StringType.
/// Is called ""_id, because the type is usually used as a compile time identifier.
template<typename char_t, char_t... Cs>
constexpr auto operator"" _id()
{
    NOTF_USING_META_NAMESPACE;
    return StringType<char, Cs...>{};
}

NOTF_CLOSE_LITERALS_NAMESPACE
