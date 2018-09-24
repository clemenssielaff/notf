#pragma once

#include <memory>

#include "../meta/config.hpp"
#include "../meta/macros.hpp"

NOTF_OPEN_NAMESPACE

// NoData type ====================================================================================================== //

/// The NoData-Trait is used for an explicit overload of reactive objects that pass no data around.
struct NoData {};

// forwards ========================================================================================================= //

NOTF_DEFINE_SHARED_POINTERS_TEMPLATE1(struct, Subscriber);
NOTF_DEFINE_SHARED_POINTERS_TEMPLATE1(class, Publisher);
NOTF_DEFINE_SHARED_POINTERS_TEMPLATE2(struct, Relay);

namespace detail {

struct SubscriberBase;

template<class>
class PublisherBase;

NOTF_DEFINE_SHARED_POINTERS_TEMPLATE1(struct, PipelineBase);

} // namespace detail

NOTF_CLOSE_NAMESPACE
