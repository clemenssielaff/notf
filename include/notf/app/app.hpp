#pragma once

#include "notf/meta/exception.hpp"

#include "notf/common/common.hpp"
#include "notf/common/tuple.hpp"

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

// can node parent ------------------------------------------------------------

template<class T, class = void>
struct has_allowed_child_types : std::false_type {};
template<class T>
struct has_allowed_child_types<T, std::void_t<typename T::allowed_child_types>> : std::true_type {};

template<class T, class = void>
struct has_forbidden_child_types : std::false_type {};
template<class T>
struct has_forbidden_child_types<T, std::void_t<typename T::forbidden_child_types>> : std::true_type {};

template<class T, class = void>
struct has_allowed_parent_types : std::false_type {};
template<class T>
struct has_allowed_parent_types<T, std::void_t<typename T::allowed_parent_types>> : std::true_type {};

template<class T, class = void>
struct has_forbidden_parent_types : std::false_type {};
template<class T>
struct has_forbidden_parent_types<T, std::void_t<typename T::forbidden_parent_types>> : std::true_type {};

template<class A, class B>
constexpr bool can_node_parent() noexcept
{
    // both A and B must be derived from Node
    if (std::negation_v<std::conjunction<std::is_base_of<Node, A>, std::is_base_of<Node, B>>>) { return false; }

    // if A has a list of explicitly allowed child types, B must be in it
    if constexpr (has_allowed_child_types<A>::value) {
        if (!is_derived_from_one_of_tuple_v<B, typename A::allowed_child_types>) { return false; }
    }
    // ... otherwise, if A has a list of explicitly forbidden child types, B must NOT be in it
    else if constexpr (has_forbidden_child_types<A>::value) {
        if (is_derived_from_one_of_tuple_v<B, typename A::forbidden_child_types>) { return false; }
    }

    // if B has a list of explicitly allowed parent types, A must be in it
    if constexpr (has_allowed_parent_types<B>::value) {
        if (!is_derived_from_one_of_tuple_v<A, typename B::allowed_parent_types>) { return false; }
    }
    // ... otherwise, if B has a list of explicitly forbidden parent types, A must NOT be in it
    else if constexpr (has_forbidden_parent_types<B>::value) {
        if (is_derived_from_one_of_tuple_v<A, typename B::forbidden_parent_types>) { return false; }
    }

    return true;
}

} // namespace detail

NOTF_CLOSE_NAMESPACE
