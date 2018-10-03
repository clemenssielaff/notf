#pragma once

#include <memory>

#include "../meta/config.hpp"
#include "../meta/macros.hpp"

NOTF_OPEN_NAMESPACE

// nodata type ====================================================================================================== //

/// The NoData-Trait is used for an explicit overload of reactive objects that pass no data around.
struct NoData {};

// forwards ========================================================================================================= //

NOTF_DEFINE_SHARED_POINTERS_TEMPLATE1(class, Subscriber);
NOTF_DEFINE_SHARED_POINTERS_TEMPLATE2(class, Publisher);
NOTF_DEFINE_SHARED_POINTERS_TEMPLATE3(class, Relay);
NOTF_DEFINE_SHARED_POINTERS_TEMPLATE2(class, Pipeline);

namespace detail {

struct SubscriberBase;
struct PublisherBase;

} // namespace detail

NOTF_CLOSE_NAMESPACE
