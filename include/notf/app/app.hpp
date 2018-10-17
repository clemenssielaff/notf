#pragma once

#include <memory>

#include "notf/meta/exception.hpp"

NOTF_OPEN_NAMESPACE

// forwards ========================================================================================================= //

// node.hpp
NOTF_DEFINE_SHARED_POINTERS(class, Node);
class NodeHandle;

// property.hpp
NOTF_DEFINE_SHARED_POINTERS(struct, AnyProperty);
NOTF_DEFINE_SHARED_POINTERS_TEMPLATE1(class, Property);
template<class T>
class PropertyHandle;

// exceptions ======================================================================================================= //

/// Exception thrown by Node- and PropertyHandles when you try to access one when it has already expired.
NOTF_EXCEPTION_TYPE(HandleExpiredError);

NOTF_CLOSE_NAMESPACE
