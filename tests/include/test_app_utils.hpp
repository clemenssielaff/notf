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
    static constexpr bool is_visible = false;
};

struct IntPropertyPolicy {
    using value_t = int;
    static constexpr StringConst name = "int";
    static constexpr value_t default_value = 123;
    static constexpr bool is_visible = true;
};

using TestNodeProperties = std::tuple< //
    FloatPropertyPolicy,               //
    IntPropertyPolicy,                 //
    BoolPropertyPolicy>;               //

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
        _create_property<bool>("bool", true, false);
        _create_property<int>("int", 123, true);
    }
};

class TestNode : public RunTimeNode {
public:
    NOTF_UNUSED TestNode(valid_ptr<Node*> parent) : RunTimeNode(parent) {}

    template<class T, class... Args>
    NOTF_UNUSED auto create_child(Args... args)
    {
        NOTF_GUARD(std::lock_guard(TheGraph::get_graph_mutex()));
        return _create_child<T>(this, std::forward<Args>(args)...);
    }
};

class LeafNodeCT : public CompileTimeNode<TestNodeProperties> {
public:
    NOTF_UNUSED LeafNodeCT(valid_ptr<Node*> parent) : CompileTimeNode<TestNodeProperties>(parent) {}
};

class TwoChildrenNode : public RunTimeNode {
public:
    NOTF_UNUSED TwoChildrenNode(valid_ptr<Node*> parent) : RunTimeNode(parent)
    {
        NOTF_GUARD(std::lock_guard(TheGraph::get_graph_mutex()));
        first_child = _create_child<LeafNodeRT>(this);
        second_child = _create_child<LeafNodeRT>(this);
    }

    TypedNodeOwner<LeafNodeRT> first_child;
    TypedNodeOwner<LeafNodeRT> second_child;
};

class ThreeChildrenNode : public RunTimeNode {
public:
    NOTF_UNUSED ThreeChildrenNode(valid_ptr<Node*> parent) : RunTimeNode(parent)
    {
        NOTF_GUARD(std::lock_guard(TheGraph::get_graph_mutex()));
        first_child = _create_child<LeafNodeRT>(this);
        second_child = _create_child<LeafNodeRT>(this);
        third_child = _create_child<LeafNodeRT>(this);
    }
    TypedNodeOwner<LeafNodeRT> first_child;
    TypedNodeOwner<LeafNodeRT> second_child;
    TypedNodeOwner<LeafNodeRT> third_child;
};

} // namespace

// accessors ======================================================================================================== //

template<>
struct notf::Accessor<TheGraph, Tester> {
    static TheGraph& get() { return TheGraph::_get(); }
    static auto freeze(std::thread::id id) { return TheGraph::_get()._freeze_guard(id); }
    static auto register_node(NodeHandle node) { return TheGraph::_get().m_node_registry.add(node); }
    static size_t get_node_count() { return TheGraph::_get().m_node_registry.get_count(); }
    static auto get_root_node(const std::thread::id thread_id) { return TheGraph::_get()._get_root_node(thread_id); }
};

template<class NodeType>
struct notf::Accessor<TypedNodeHandle<NodeType>, Tester> {
    static std::shared_ptr<NodeType> get_shared_ptr(const TypedNodeHandle<NodeType>& handle)
    {
        return handle.m_node.lock();
    }
};

template<class T>
struct notf::Accessor<PropertyHandle<T>, Tester> {
    static std::shared_ptr<Property<T>> get_shared_ptr(const PropertyHandle<T>& handle)
    {
        return handle.m_property.lock();
    }
};

template<>
struct notf::Accessor<Node, Tester> {

    Accessor(Node& node) : m_node(node) {}

    template<class NodeType>
    Accessor(TypedNodeHandle<NodeType>& node)
        : m_node(*(TypedNodeHandle<NodeType>::template AccessFor<Tester>::get_shared_ptr(node).get()))
    {}

    template<class NodeType>
    Accessor(TypedNodeOwner<NodeType>& node)
        : m_node(*(TypedNodeHandle<NodeType>::template AccessFor<Tester>::get_shared_ptr(node).get()))
    {}

    size_t get_property_hash() const { return m_node._calculate_property_hash(); }

    bool is_dirty() const
    {
        NOTF_GUARD(std::lock_guard(TheGraph::get_graph_mutex()));
        return m_node._is_flag_set(to_number(Node::InternalFlags::DIRTY));
    }

    void set_uuid(const Uuid& uuid) { const_cast<Uuid&>(m_node.m_uuid) = uuid; }

    bool has_ancestor(const Node* node) const { return m_node.has_ancestor(node); }

    bool is_user_flag_set(const size_t index, const std::thread::id thread_id = std::this_thread::get_id()) const
    {
        return m_node._is_flag_set(index + Node::s_internal_flag_count, thread_id);
    }

    void remove_child(NodeHandle handle) { m_node._remove_child(handle); }

    void set_parent(NodeHandle parent)
    {
        NOTF_GUARD(std::lock_guard(TheGraph::get_graph_mutex()));
        m_node._set_parent(parent);
    }

    NodeHandle get_parent(std::thread::id thread_id = std::this_thread::get_id())
    {
        Node* parent = m_node._get_parent(thread_id);
        if (parent) { return parent->shared_from_this(); }
        return {};
    }

    size_t get_child_count(std::thread::id thread_id = std::this_thread::get_id())
    {
        NOTF_GUARD(std::lock_guard(TheGraph::get_graph_mutex()));
        return m_node._read_children(thread_id).size();
    }

    Node& m_node;
};

template<class NodeType>
auto to_shared_ptr(TypedNodeHandle<NodeType>& node)
{
    return TypedNodeHandle<NodeType>::template AccessFor<Tester>::get_shared_ptr(node);
}
template<class T>
auto to_shared_ptr(PropertyHandle<T>& property)
{
    return PropertyHandle<T>::template AccessFor<Tester>::get_shared_ptr(property);
}
