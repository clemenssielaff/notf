#pragma once

#include <memory>

#include "notf/meta/macros.hpp"
#include "notf/meta/types.hpp"

NOTF_OPEN_NAMESPACE

// forwards ========================================================================================================= //

NOTF_DECLARE_SHARED_POINTERS(struct, AnySubscriber);
NOTF_DECLARE_SHARED_POINTERS(struct, AnyPublisher);
NOTF_DECLARE_SHARED_POINTERS(struct, AnyOperator);

NOTF_DECLARE_SHARED_POINTERS_TEMPLATE1(class, Subscriber);
NOTF_DECLARE_SHARED_POINTERS_TEMPLATE2(class, Publisher);
NOTF_DECLARE_SHARED_POINTERS_TEMPLATE3(class, Operator);
NOTF_DECLARE_SHARED_POINTERS_TEMPLATE1(class, Pipeline);

NOTF_DECLARE_UNIQUE_POINTERS(class, AnyPipeline);

NOTF_CLOSE_NAMESPACE
