#pragma once

#include "notf/meta/hash.hpp"
#include "notf/meta/integer.hpp"

NOTF_OPEN_NAMESPACE

// string type ====================================================================================================== //

/// Type defined by a string literal at compile time.
template<char... Cs>
struct StringType final {

    template<char... Os>
    friend struct StringType; // to allow other StringTypes to access the private `s_text` member field.

    // types ----------------------------------------------------------------------------------- //
public:
    static constexpr auto Chars = {Cs...};

    // methods --------------------------------------------------------------------------------- //
public:
    /// The string literal that defines this type.
    static constexpr const char* c_str() noexcept { return s_text; }

    /// The numbers of letters in the TypeString without the closing null.
    static constexpr size_t get_size() noexcept { return sizeof...(Cs); }

    /// Return a view on this string.
    static std::string_view get_view() noexcept { return std::string_view(c_str(), get_size()); }

    /// The hash of the string.
    static constexpr size_t get_hash() noexcept { return hash_string(s_text, get_size()); }

    /// Tests if two TypeStrings are the same..
    template<class Other, typename Indices = std::make_index_sequence<min(get_size(), Other::get_size())>>
    static constexpr bool is_same() noexcept {
        if constexpr (get_size() != Other::get_size()) { return false; }
        return _is_same_impl<Other>(Indices{});
    }
    template<class Other>
    static constexpr bool is_same(Other) noexcept {
        return is_same<Other>();
    }

    /// Access to an individual character of the string.
    static constexpr char at(const size_t index) {
        if (index < get_size()) {
            return s_text[index];
        } else {
            throw std::out_of_range("Failed to read out-of-range StringType character");
        }
    }

private:
    /// Helper function folding a character comparison over the complete string.
    template<class Other, size_t... I>
    static constexpr bool _is_same_impl(std::index_sequence<I...>) noexcept {
        return ((s_text[I] == Other::s_text[I]) && ...);
    }

    // fields ---------------------------------------------------------------------------------- //
private:
    /// C-string compile-time access to the characters passed as template arguments.
    inline static constexpr char const s_text[sizeof...(Cs) + 1] = {Cs..., '\0'};
};

// string const ======================================================================================================//

/// Compile time string.
struct ConstString {

    // methods --------------------------------------------------------------------------------- //
public:
    /// Constructor
    /// Takes a compile time literal.
    template<size_t N>
    constexpr ConstString(const char (&a)[N]) noexcept : m_text(a), m_size(N - 1) {}

    /// The constant string literal.
    constexpr const char* c_str() const noexcept { return m_text; }

    /// The numbers of letters in the string without the closing null.
    constexpr size_t get_size() const noexcept { return m_size; }

    /// Return a view on this string.
    std::string_view get_view() const noexcept { return std::string_view(c_str(), get_size()); }

    /// The hash of the string.
    constexpr size_t get_hash() const noexcept { return hash_string(m_text, m_size); }

    /// operator ==
    constexpr bool operator==(const ConstString& other) const noexcept {
        if (other.get_size() != get_size()) { return false; }
        for (size_t i = 0; i < get_size(); ++i) {
            if (other[i] != m_text[i]) { return false; }
        }
        return true;
    }

    /// Access to an individual character of the string.
    constexpr char operator[](const size_t index) const {
        return (index < m_size) ? m_text[index] :
                                  throw std::out_of_range("Failed to read out-of-range ConstString character");
    }

    // fields ---------------------------------------------------------------------------------- //
private:
    /// Pointer to the text stored somewhere with linkage.
    const char* const m_text;

    /// Numbers of letters in the string without the closing null.
    const size_t m_size;
};

// comparison ======================================================================================================= //

/// Comparison between a ConstString and a StringType.
template<char... Cs>
constexpr bool operator==(const ConstString& lhs, const StringType<Cs...>& rhs) noexcept {
    if (sizeof...(Cs) != lhs.get_size()) { return false; }
    for (size_t i = 0; i < lhs.get_size(); ++i) {
        if (rhs.at(i) != lhs[i]) { return false; }
    }
    return true;
}
template<char... Cs>
constexpr bool operator==(const StringType<Cs...>& lhs, const ConstString& rhs) noexcept {
    return rhs == lhs;
}

// make string type ================================================================================================= //

namespace detail {

template<class T>
auto concat_string_type_impl(T&& last) noexcept // recursion breaker
{
    return last;
}
template<char... Ls, char... Rs, class... Tail>
auto concat_string_type_impl(StringType<Ls...>, StringType<Rs...>, Tail... tail) noexcept {
    return concat_string_type_impl(StringType<Ls..., Rs...>{}, std::forward<Tail>(tail)...);
}

template<const ConstString& string, size_t... I>
constexpr auto const_string_to_string_type(std::index_sequence<I...>) noexcept {
    return StringType<string[I]...>{};
}

template<char number>
constexpr auto digit_to_string_type() noexcept {
    static_assert(number <= 9);
    return StringType<static_cast<char>(48 + number)>{};
}

template<size_t number, size_t... I>
constexpr auto number_to_string_type(std::index_sequence<I...>) noexcept {
    constexpr size_t last_index = sizeof...(I) - 1; // reverse the expansion
    return concat_string_type_impl(digit_to_string_type<get_digit(number, last_index - I)>()...);
}

} // namespace detail

/// Creates a new StringType from a ConstString or integral literal.
/// Note that the ConstString value that is passed must have linkage.
/// Example:
///   constexpr ConstString test = "test";
///   using TestType = decltype(make_string_type<test>());
template<const ConstString& string>
constexpr auto make_string_type() noexcept {
    return detail::const_string_to_string_type<string>(std::make_index_sequence<string.get_size()>{});
}
template<size_t number>
constexpr auto make_string_type_from_number() noexcept {
    return detail::number_to_string_type<number>(std::make_index_sequence<count_digits(number)>{});
}
#ifndef NOTF_MSVC
// (yet another) MSVC bug causes an internal compiler error if you have two functions named `make_string_type`, that
// only differ in their template argument type, even though it seems to be valid c++ code.
template<size_t number>
constexpr auto make_string_type() noexcept {
    return make_string_type_from_number<number>();
}
#endif

/// Helper typdef for defining a StringType with a *constexpr* value.
/// Example:
///
///     static constexpr ConstString example_string = "example";
///     static constexpr size_t example_number = 123;
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

// literals ========================================================================================================= //

#ifndef NOTF_MSVC
NOTF_OPEN_LITERALS_NAMESPACE
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wgnu-string-literal-operator-template"

/// User defined literal, turning any string literal into a StringType.
/// Is called ""_id, because the type is usually used as a compile time identifier.
template<typename char_t, char_t... Cs>
constexpr auto operator"" _id() {
    return StringType<Cs...>{};
}

#pragma clang diagnostic pop
NOTF_CLOSE_LITERALS_NAMESPACE
#endif // not defined NOTF_MSVC

NOTF_CLOSE_NAMESPACE
