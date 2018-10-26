#pragma once

#include "notf/app/root_node.hpp"

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

struct IntPropertyPolicy {
    using value_t = int;
    static constexpr StringConst name = "int";
    static constexpr value_t default_value = 123;
    static constexpr bool is_visible = true;
};

struct TestNodePolicy {
    using properties = std::tuple<                //
        CompileTimeProperty<FloatPropertyPolicy>, //
        CompileTimeProperty<IntPropertyPolicy>,   //
        CompileTimeProperty<BoolPropertyPolicy>>; //
};

class TestRootNode : public RunTimeRootNode {
    using allowed_child_types = std::tuple<>; // hide `allowed_child_types` definition
public:
    NOTF_UNUSED TestRootNode() = default;

    template<class T, class... Args>
    NOTF_UNUSED auto create_child(Args... args)
    {
        NOTF_GUARD(std::lock_guard(TheGraph::get_graph_mutex()));
        return _create_child<T>(this, std::forward<Args>(args)...);
    }
};

class LeafNodeRT : public RunTimeNode {
public:
    NOTF_UNUSED LeafNodeRT(valid_ptr<Node*> parent) : RunTimeNode(parent)
    {
        _create_property<float>("float", 0.123f, true);
        _create_property<bool>("bool", true, true);
        _create_property<int>("int", 123, true);
    }
};

class LeafNodeCT : public CompileTimeNode<TestNodePolicy> {
public:
    NOTF_UNUSED LeafNodeCT(valid_ptr<Node*> parent) : CompileTimeNode<TestNodePolicy>(parent) {}
};

class SingleChildNode : public RunTimeNode {
public:
    NOTF_UNUSED SingleChildNode(valid_ptr<Node*> parent) : RunTimeNode(parent)
    {
        NOTF_GUARD(std::lock_guard(TheGraph::get_graph_mutex()));
        first_child = _create_child<LeafNodeRT>(this);
    }
    NodeOwner first_child;
};

class TwoChildrenNode : public RunTimeNode {
public:
    NOTF_UNUSED TwoChildrenNode(valid_ptr<Node*> parent) : RunTimeNode(parent)
    {
        NOTF_GUARD(std::lock_guard(TheGraph::get_graph_mutex()));
        first_child = _create_child<LeafNodeRT>(this);
        second_child = _create_child<LeafNodeRT>(this);
    }
    NodeOwner first_child;
    NodeOwner second_child;
};

} // namespace

// accessors ======================================================================================================== //

template<>
struct notf::Accessor<Node, Tester> {
    Accessor(Node& node) : m_node(node) {}
    size_t get_property_hash() const { return m_node._calculate_property_hash(); }
    bool is_dirty() const
    {
        NOTF_GUARD(std::lock_guard(TheGraph::get_graph_mutex()));
        return m_node._is_flag_set(to_number(Node::InternalFlags::DIRTY));
    }
    Node& m_node;
};

template<>
struct notf::Accessor<TheGraph, Tester> {
    static TheGraph& get() { return TheGraph::_get(); }
    static size_t get_node_count() { return TheGraph::_get().m_node_registry.get_count(); }
    static auto freeze(std::thread::id id) { return TheGraph::_get()._freeze_guard(id); }
};

template<class NodeType>
struct notf::Accessor<TypedNodeHandle<NodeType>, Tester> {
    static std::shared_ptr<NodeType> get_shared_ptr(const TypedNodeHandle<NodeType>& handle)
    {
        return handle.m_node.lock();
    }
};
