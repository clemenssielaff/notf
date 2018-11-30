#pragma once

#include <memory>

#include "notf/meta/fwd.hpp"
#include "notf/meta/macros.hpp"

NOTF_OPEN_NAMESPACE

// forwards ========================================================================================================= //

// arithmetic.hpp
namespace detail {
template<class, class, class, size_t>
struct Arithmetic;
} // namespace detail

// msgpack.hpp
class MsgPack;

// mutex.hpp
class Mutex;
class RecursiveMutex;

// thread.hpp
class Thread;

// uuid.hpp
class Uuid;

NOTF_CLOSE_NAMESPACE
