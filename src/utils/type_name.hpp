#pragma once

#include <string>

#include "common/meta.hpp"

NOTF_OPEN_NAMESPACE

namespace detail {

/// Demangles a mangled type name.
/// @param name     Type name to demangle.
/// @returns        Pretty printed name, if this plattform is supported.
std::string demangle_type_name(const char* name);

} // namespace detail

/// Utility function to get a human-readable type name.
/// Code from:
///     https://stackoverflow.com/a/4541470
/// @param t    Instance of the type in question.
/// @returns    Pretty printed name, if this plattform is supported.
template<class T>
inline std::string type_name()
{
    return detail::demangle_type_name(typeid(T).name());
}
template<class T>
inline std::string type_name(const T& t)
{
    return detail::demangle_type_name(typeid(t).name());
}
template<>
inline std::string type_name(const std::type_info& type_info)
{
    return detail::demangle_type_name(type_info.name());
}

NOTF_CLOSE_NAMESPACE
