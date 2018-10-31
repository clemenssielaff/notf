#pragma once

#include <string>

#ifdef __GNUG__
#include <cstdlib>
#include <cxxabi.h>
#include <memory>
#endif

#include "notf/meta/config.hpp"

NOTF_OPEN_NAMESPACE

// type_name ======================================================================================================== //

namespace detail {
#ifdef __GNUG__

/// Demangles a mangled type name.
/// @param name     Type name to demangle.
/// @returns        Pretty printed name, if this plattform is supported.
inline std::string demangle_type_name(const char* name)
{
    // Possible `status` values after a call to `abi::__cxa_demangle`
    //  0: The demangling operation succeeded.
    // -1: A memory allocation failiure occurred.
    // -2: mangled_name is not a valid name under the C++ ABI mangling rules.
    // -3: One of the arguments is invalid.
    int status = 1;
    std::unique_ptr<char, void (*)(void*)> res{abi::__cxa_demangle(name, nullptr, nullptr, &status), std::free};
    return (status == 0) ? res.get() : name;
}

#else // ifdef __GNUG__

// Does nothing if this system is not supported
inline std::string demangle_type_name(const char* name) { return name; }

#endif // ifdef __GNUG__
} // namespace detail

/// Utility function to get a human-readable type name.
/// Code from:
///     https://stackoverflow.com/a/4541470
/// @param t    Instance of the type in question.
/// @returns    Pretty printed name, if this plattform is supported.
template<class T>
std::string type_name()
{
    return detail::demangle_type_name(typeid(T).name());
}
template<class T>
std::string type_name(const T& type)
{
    return detail::demangle_type_name(typeid(type).name());
}
template<>
inline std::string type_name(const std::type_info& type)
{
    return detail::demangle_type_name(type.name());
}

NOTF_CLOSE_NAMESPACE
