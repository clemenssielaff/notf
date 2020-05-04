#include "notf/app/graph/property.hpp"

#include "notf/meta/log.hpp"

NOTF_OPEN_NAMESPACE

// property operator ================================================================================================ //

namespace detail {

void report_property_operator_error(const std::exception& exception) {
    NOTF_LOG_ERROR("Caught and ignored error propagated to a Property Operator: {}", exception.what());
}

} // namespace detail

NOTF_CLOSE_NAMESPACE
