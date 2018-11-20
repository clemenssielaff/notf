#pragma once

#include <memory>

#include "notf/meta/macros.hpp"

NOTF_OPEN_NAMESPACE

// forwards ========================================================================================================= //

// arithmetic.hpp
namespace detail {
template<class, class, class, size_t>
struct Arithmetic;
} // namespace detail

// timer_pool.hpp
class TheTimerPool;
NOTF_DEFINE_SHARED_POINTERS(class, Timer);
NOTF_DEFINE_SHARED_POINTERS(class, OneShotTimer);
NOTF_DEFINE_SHARED_POINTERS(class, IntervalTimer);
NOTF_DEFINE_SHARED_POINTERS(class, VariableTimer);

// msgpack.hpp
class MsgPack;

// thread.hpp
class Thread;

// uuid.hpp
class Uuid;

NOTF_CLOSE_NAMESPACE
