#pragma once

#include "common/meta.hpp"

#ifdef NOTF_CPP17

#include <string_view>
NOTF_OPEN_NAMESPACE
using string_view = std::basic_string_view<char, std::char_traits<char>>;
using u16string_view = std::basic_string_view<char16_t, std::char_traits<char16_t>>;
using u32string_view = std::basic_string_view<char32_t, std::char_traits<char32_t>>;
using wstring_view = std::basic_string_view<wchar_t, std::char_traits<wchar_t>>;
NOTF_CLOSE_NAMESPACE

#else

#include "cpp17_headers/string_view.hpp"
NOTF_OPEN_NAMESPACE
using string_view = stx::basic_string_view<char, std::char_traits<char>>;
using u16string_view = stx::basic_string_view<char16_t, std::char_traits<char16_t>>;
using u32string_view = stx::basic_string_view<char32_t, std::char_traits<char32_t>>;
using wstring_view = stx::basic_string_view<wchar_t, std::char_traits<wchar_t>>;
NOTF_CLOSE_NAMESPACE

#endif
