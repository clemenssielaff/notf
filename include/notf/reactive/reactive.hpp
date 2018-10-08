#pragma once

#include <memory>

#include "../meta/macros.hpp"
#include "../meta/types.hpp"

NOTF_OPEN_NAMESPACE

// forwards ========================================================================================================= //

struct UntypedSubscriber;
struct UntypedPublisher;

NOTF_DEFINE_SHARED_POINTERS_TEMPLATE1(class, Subscriber);
NOTF_DEFINE_SHARED_POINTERS_TEMPLATE2(class, Publisher);
NOTF_DEFINE_SHARED_POINTERS_TEMPLATE3(class, Relay);
NOTF_DEFINE_SHARED_POINTERS_TEMPLATE2(class, Pipeline);

NOTF_CLOSE_NAMESPACE
