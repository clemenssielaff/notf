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
namespace detail {
struct AnyNodeHandle;
}
template<class>
class TypedNodeHandle;
template<class>
class TypedNodeOwner;
using NodeHandle = TypedNodeHandle<Node>;
using NodeOwner = TypedNodeOwner<Node>;

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

// is compile time node ============================================================================================= //

namespace detail {

struct CompileTimeNodeIdentifier {
    template<class T>
    static constexpr auto test()
    {
        if constexpr (decltype(_has_policy_t<T>(std::declval<T>()))::value) {
            using policy_t = typename T::policy_t;
            return std::is_convertible<T*, CompileTimeNode<policy_t>*>{};
        }
        else {
            return std::false_type{};
        }
    }

private:
    template<class T>
    static constexpr auto _has_policy_t(const T&) -> decltype(typename T::policy_t{}, std::true_type{});
    template<class>
    static constexpr auto _has_policy_t(...) -> std::false_type;
};

/// Struct derived either from std::true_type or std::false type, depending on whether T is a CompileTimeNode or not.
template<class T>
struct is_compile_time_node : decltype(CompileTimeNodeIdentifier::test<T>()) {};

/// Constexpr boolean that is true only if T is a CompileTimeNode.
template<class T>
static constexpr bool is_compile_time_node_v = is_compile_time_node<T>::value;

} // namespace detail

NOTF_CLOSE_NAMESPACE
