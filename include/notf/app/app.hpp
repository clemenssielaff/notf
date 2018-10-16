#pragma once

#include <memory>

#include "notf/meta/exception.hpp"

NOTF_OPEN_NAMESPACE

// forwards ========================================================================================================= //

// property.hpp
NOTF_DEFINE_SHARED_POINTERS_TEMPLATE1(class, Property);

// exceptions ======================================================================================================= //

NOTF_EXCEPTION_TYPE(ExpiredHandleError);

NOTF_CLOSE_NAMESPACE
