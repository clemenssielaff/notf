#pragma once

#include "notf/common/common.hpp"
#include "notf/meta/exception.hpp"

NOTF_OPEN_NAMESPACE

// forwards ========================================================================================================= //

// node.hpp
NOTF_DEFINE_SHARED_POINTERS(class, Node);
class NodeHandle;

// node_compiletime.hpp
NOTF_DEFINE_SHARED_POINTERS_TEMPLATE1(class, CompileTimeNode);

// node_root.hpp
class RootNode;

// property.hpp
NOTF_DEFINE_SHARED_POINTERS(class, AnyProperty);
NOTF_DEFINE_SHARED_POINTERS_TEMPLATE1(class, Property);
template<class T>
class PropertyHandle;

// exceptions ======================================================================================================= //

/// Exception thrown by Node- and PropertyHandles when you try to access one when it has already expired.
NOTF_EXCEPTION_TYPE(HandleExpiredError);

NOTF_CLOSE_NAMESPACE
