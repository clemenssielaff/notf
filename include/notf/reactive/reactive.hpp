#pragma once

#include <memory>

#include "../meta/config.hpp"
#include "../meta/macros.hpp"

// reactive namespace =============================================================================================== //

/// Name of the reactive namespace
#define NOTF_REACTIVE_NAMESPACE_NAME reactive

/// Opens the reactive namespace.
#define NOTF_OPEN_REACTIVE_NAMESPACE NOTF_OPEN_NAMESPACE namespace NOTF_REACTIVE_NAMESPACE_NAME {

/// For visual balance with NOTF_OPEN_REACTIVE_NAMESPACE.
#define NOTF_CLOSE_REACTIVE_NAMESPACE NOTF_CLOSE_NAMESPACE }

/// Use the versioned namespace.
#define NOTF_USING_REACTIVE_NAMESPACE using namespace ::NOTF_NAMESPACE_NAME::NOTF_REACTIVE_NAMESPACE_NAME

// forwards ========================================================================================================= //

NOTF_DEFINE_SHARED_POINTERS_TEMPLATE1(struct, Consumer);
NOTF_DEFINE_SHARED_POINTERS_TEMPLATE1(struct, Publisher);
NOTF_DEFINE_SHARED_POINTERS_TEMPLATE2(struct, Relay);

namespace detail {

NOTF_DEFINE_SHARED_POINTERS_TEMPLATE1(struct, PipelineBase);

struct ConsumerBase;

template<class>
struct PublisherBase;

} // namespace detail

// ================================================================================================================== //

/// The NoData-Trait is used for an explicit overload of reactive objects that pass no data around.
struct NoData {};
