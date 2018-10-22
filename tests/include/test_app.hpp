#pragma once

#include "notf/app/node_compiletime.hpp"
#include "notf/app/node_runtime.hpp"

NOTF_USING_NAMESPACE;

namespace {

struct FloatPropertyPolicy {
    using value_t = float;
    static constexpr StringConst name = "float";
    static constexpr value_t default_value = 0.123f;
    static constexpr bool is_visible = true;
};

struct BoolPropertyPolicy {
    using value_t = bool;
    static constexpr StringConst name = "bool";
    static constexpr value_t default_value = true;
    static constexpr bool is_visible = true;
};

struct TestNodePolicy {
    using properties = std::tuple<                //
        CompileTimeProperty<FloatPropertyPolicy>, //
        CompileTimeProperty<BoolPropertyPolicy>>; //
};
using CompileTimeTestNode = CompileTimeNode<TestNodePolicy>;

//class LeafNode : public CompileTimeNode<TestNodePolicy> {
//    NOTF_UNUSED LeafNode() : CompileTimeNode<TestNodePolicy>() {}
//};

//class SingleChildNode : public CompileTimeNode<TestNodePolicy> {
//    NOTF_UNUSED SingleChildNode() : CompileTimeNode<TestNodePolicy>() { first_child = _create_child<LeafNode>(); }
//    NodeHandle first_child;
//};

//class TwoChildNode : public CompileTimeNode<TestNodePolicy> {
//    NOTF_UNUSED TwoChildNode() : CompileTimeNode<TestNodePolicy>()
//    {
//        first_child = _create_child<LeafNode>();
//        second_child = _create_child<LeafNode>();
//    }
//    NodeHandle first_child;
//    NodeHandle second_child;
//};

} // namespace

template<>
struct notf::Accessor<Node, Tester> {
    Accessor(Node& node) : m_node(node) {}

    size_t get_property_hash() const { return m_node._calculate_property_hash(); }

    Node& m_node;
};
