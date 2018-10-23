#pragma once

#include "notf/common/common.hpp"
#include "notf/meta/exception.hpp"

NOTF_OPEN_NAMESPACE

// forwards ========================================================================================================= //

// graph.hpp
class TheGraph;

// node.hpp
NOTF_DEFINE_SHARED_POINTERS(class, Node);

// node_compiletime.hpp
template<class>
class CompileTimeNode;

// node_handle.hpp
class NodeHandle;
class NodeMasterHandle;

// node_runtime.hpp
class RunTimeNode;

// property.hpp
NOTF_DEFINE_SHARED_POINTERS(class, AnyProperty);
NOTF_DEFINE_SHARED_POINTERS_TEMPLATE1(class, Property);
template<class>
class RunTimeProperty;
template<class>
class CompileTimeProperty;

// property_handle.hpp
template<class>
class PropertyHandle;

// root_node.hpp
NOTF_DEFINE_SHARED_POINTERS(class, AnyRootNode);
class RunTimeRootNode;
template<class>
class CompileTimeRootNode;

// scene.hpp
class Scene;

// widget.hpp
class Widget;

// window.hpp
class Window;

// exceptions ======================================================================================================= //

/// Exception thrown by Node- and PropertyHandles when you try to access one when it has already expired.
NOTF_EXCEPTION_TYPE(HandleExpiredError);

NOTF_CLOSE_NAMESPACE
