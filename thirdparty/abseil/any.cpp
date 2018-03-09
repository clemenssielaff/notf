#include "abseil/any.hpp"

#ifndef NOTF_CPP17

namespace absl {

bad_any_cast::~bad_any_cast() noexcept = default;

any::ObjInterface::~ObjInterface() = default;

} // namespace absl

#endif
