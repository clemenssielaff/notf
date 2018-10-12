#pragma once

#include <memory>

#include "notf/meta/macros.hpp"
#include "notf/meta/types.hpp"

NOTF_OPEN_NAMESPACE

// forwards ========================================================================================================= //

struct AnySubscriber;
struct AnyPublisher;

NOTF_DEFINE_SHARED_POINTERS_TEMPLATE1(class, Subscriber);
NOTF_DEFINE_SHARED_POINTERS_TEMPLATE2(class, Publisher);
NOTF_DEFINE_SHARED_POINTERS_TEMPLATE3(class, Operator);
NOTF_DEFINE_SHARED_POINTERS_TEMPLATE2(class, Pipeline);

NOTF_CLOSE_NAMESPACE
