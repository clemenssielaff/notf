#include "utils/type_name.hpp"

#ifdef __GNUG__
#include <cstdlib>
#include <cxxabi.h>
#include <memory>

NOTF_OPEN_NAMESPACE
namespace detail {

std::string demangle_type_name(const char* name)
{
    int status = -1;
    std::unique_ptr<char, void (*)(void*)> res{abi::__cxa_demangle(name, nullptr, nullptr, &status), std::free};
    return (status == 0) ? res.get() : name;
}

} // namespace detail
NOTF_CLOSE_NAMESPACE

#else

NOTF_OPEN_NAMESPACE
namespace detail {

// Does nothing if this system is not supported
std::string demangle_type_name(const char* name) { return name; }

} // namespace detail
NOTF_CLOSE_NAMESPACE

#endif
