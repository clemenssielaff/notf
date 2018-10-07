#pragma once

#include <memory>

#include "../meta/macros.hpp"

NOTF_OPEN_NAMESPACE

// forwards ========================================================================================================= //

// timer_pool.hpp
class TheTimerPool;
NOTF_DEFINE_SHARED_POINTERS(class, Timer);
NOTF_DEFINE_SHARED_POINTERS(class, OneShotTimer);
NOTF_DEFINE_SHARED_POINTERS(class, IntervalTimer);
NOTF_DEFINE_SHARED_POINTERS(class, VariableTimer);

// msgpack.hpp
class MsgPack;

NOTF_CLOSE_NAMESPACE
