#pragma once

#include "notf/app/application.hpp"
#include "notf/app/node_runtime.hpp"
#include "notf/app/root_node.hpp"

NOTF_OPEN_NAMESPACE

// accessors ======================================================================================================== //

/// Node Handle
template<class NodeType>
struct Accessor<TypedNodeHandle<NodeType>, Tester> {
    static std::shared_ptr<NodeType> to_shared_ptr(const TypedNodeHandle<NodeType>& handle) {
        return handle.m_node.lock();
    }
};
template<class NodeType>
auto to_shared_ptr(TypedNodeHandle<NodeType> node) {
    return TypedNodeHandle<NodeType>::template AccessFor<Tester>::to_shared_ptr(node);
}

/// Node
template<>
struct Accessor<Node, Tester> {

    Accessor(Node& node) : m_node(node) {}

    template<class NodeType>
    Accessor(TypedNodeHandle<NodeType>& node)
        : m_node(*(TypedNodeHandle<NodeType>::template AccessFor<Tester>::to_shared_ptr(node).get())) {}

    template<class NodeType>
    Accessor(TypedNodeOwner<NodeType>& node)
        : m_node(*(TypedNodeHandle<NodeType>::template AccessFor<Tester>::to_shared_ptr(node).get())) {}

    size_t get_property_hash() const { return m_node._calculate_property_hash(); }

    //    bool is_dirty() const {
    //        NOTF_GUARD(std::lock_guard(TheGraph()->get_graph_mutex()));
    //        return m_node._get_internal_flag(to_number(Node::InternalFlags::DIRTY));
    //    }

    void set_uuid(const Uuid& uuid) { const_cast<Uuid&>(m_node.m_uuid) = uuid; }

    bool has_ancestor(const Node* node) const { return m_node.has_ancestor(node); }

    static constexpr size_t get_user_flag_count() noexcept { return Node::s_user_flag_count; }

    //    bool get_flag(const size_t index, const std::thread::id thread_id = std::this_thread::get_id()) const {
    //        return m_node._get_internal_flag(index + Node::s_internal_flag_count, thread_id);
    //    }

    template<class T, class... Args>
    auto create_child(Args... args) {
        return m_node._create_child<T>(&m_node, std::forward<Args>(args)...);
    }

    void remove_child(NodeHandle handle) { m_node._remove_child(handle); }

    //    void set_parent(NodeHandle parent) {
    //        NOTF_GUARD(std::lock_guard(TheGraph()->get_graph_mutex()));
    //        m_node._set_parent(parent);
    //    }

    //    NodeHandle get_parent(std::thread::id thread_id = std::this_thread::get_id()) {
    //        Node* parent = m_node._get_parent(thread_id);
    //        if (parent) { return parent->shared_from_this(); }
    //        return {};
    //    }

    //    size_t get_child_count(std::thread::id thread_id = std::this_thread::get_id()) {
    //        NOTF_GUARD(std::lock_guard(TheGraph()->get_graph_mutex()));
    //        return m_node._read_children(thread_id).size();
    //    }

    Node& m_node;
};

/// Graph
template<>
struct Accessor<detail::Graph, Tester> {
    //    static detail::Graph& get() { return *TheGraph(); }
    static auto register_node(NodeHandle node) { return TheGraph()->m_node_registry.add(to_shared_ptr(node)); }
};

// compile time test node =========================================================================================== //

namespace detail {

// properties =================================================================

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

// slots ======================================================================

struct NoneSlot {
    using value_t = None;
    static constexpr StringConst name = "to_none";
};

struct IntSlot {
    using value_t = int;
    static constexpr StringConst name = "to_int";
};

// signals ====================================================================

struct NoneSignal {
    using value_t = None;
    static constexpr StringConst name = "on_none";
};

struct IntSignal {
    using value_t = int;
    static constexpr StringConst name = "on_int";
};

// policy =====================================================================

struct TestNodePolicy {
    using properties = std::tuple< //
        FloatPropertyPolicy,       //
        IntPropertyPolicy,         //
        BoolPropertyPolicy>;       //

    using slots = std::tuple< //
        NoneSlot,             //
        IntSlot>;             //

    using signals = std::tuple< //
        NoneSignal,             //
        IntSignal               //
        >;                      //
};

} // namespace detail

class TestNodeCT : public CompileTimeNode<detail::TestNodePolicy> {
public:
    TestNodeCT(valid_ptr<Node*> parent) : CompileTimeNode<detail::TestNodePolicy>(parent) {}
};

// run time test node =============================================================================================== //

class TestNodeRT : public RunTimeNode {
public:
    TestNodeRT(valid_ptr<Node*> parent) : RunTimeNode(parent) {
        _create_property<float>("float", 0.123f, true);
        _create_property<bool>("bool", true, false);
        _create_property<int>("int", 123, true);

        _create_slot<None>("to_none");
        _create_slot<int>("to_int");

        _create_signal<None>("on_none");
        _create_signal<int>("on_int");
    }
};

// functions ======================================================================================================== //

struct TheRootNode {

    TheRootNode() : m_root(TypedNodeHandle<RootNode>::AccessFor<Tester>::to_shared_ptr(TheGraph()->get_root_node())) {}

    template<class T, class... Args>
    auto create_child(Args... args) {
        return Node::AccessFor<Tester>(*m_root).create_child<T>(std::forward<Args>(args)...);
    }

private:
    std::shared_ptr<RootNode> m_root;
};

// namespace {

// struct FloatPropertyPolicy {
//    using value_t = float;
//    static constexpr StringConst name = "float";
//    static constexpr value_t default_value = 0.123f;
//    static constexpr bool is_visible = true;
//};

// struct BoolPropertyPolicy {
//    using value_t = bool;
//    static constexpr StringConst name = "bool";
//    static constexpr value_t default_value = true;
//    static constexpr bool is_visible = false;
//};

// struct IntPropertyPolicy {
//    using value_t = int;
//    static constexpr StringConst name = "int";
//    static constexpr value_t default_value = 123;
//    static constexpr bool is_visible = true;
//};

// struct TestNodePolicy {
//    using properties = std::tuple< //
//        FloatPropertyPolicy,       //
//        IntPropertyPolicy,         //
//        BoolPropertyPolicy>;       //
//};

// class LeafNodeCT : public CompileTimeNode<TestNodePolicy> {
// public:
//    NOTF_UNUSED LeafNodeCT(valid_ptr<Node*> parent) : CompileTimeNode<TestNodePolicy>(parent) {}
//};

// class LeafNodeRT : public RunTimeNode {
// public:
//    NOTF_UNUSED LeafNodeRT(valid_ptr<Node*> parent) : RunTimeNode(parent) {
//        _create_property<float>("float", 0.123f, true);
//        _create_property<bool>("bool", true, false);
//        _create_property<int>("int", 123, true);
//    }
//};

// class TestNode : public RunTimeNode {
// public:
//    NOTF_UNUSED TestNode() : RunTimeNode(this) {}
//    NOTF_UNUSED TestNode(valid_ptr<Node*> parent) : RunTimeNode(parent) {}

//    NOTF_UNUSED bool get_flag(size_t index) {
//        NOTF_GUARD(std::lock_guard(TheGraph()->get_graph_mutex()));

//        return _get_flag(index);
//    }
//    NOTF_UNUSED void set_flag(size_t index, bool value = true) {
//        NOTF_GUARD(std::lock_guard(TheGraph()->get_graph_mutex()));
//        _set_flag(index, value);
//    }

//    template<class T, class... Args>
//    NOTF_UNUSED auto create_child(Args... args) {
//        NOTF_GUARD(std::lock_guard(TheGraph()->get_graph_mutex()));
//        return _create_child<T>(this, std::forward<Args>(args)...);
//    }
//};

// class TwoChildrenNode : public RunTimeNode {
// public:
//    NOTF_UNUSED TwoChildrenNode(valid_ptr<Node*> parent) : RunTimeNode(parent) {
//        NOTF_GUARD(std::lock_guard(TheGraph()->get_graph_mutex()));
//        first_child = _create_child<LeafNodeRT>(this);
//        second_child = _create_child<LeafNodeRT>(this);
//    }

//    TypedNodeOwner<LeafNodeRT> first_child;
//    TypedNodeOwner<LeafNodeRT> second_child;
//};

// class ThreeChildrenNode : public RunTimeNode {
// public:
//    NOTF_UNUSED ThreeChildrenNode(valid_ptr<Node*> parent) : RunTimeNode(parent) {
//        NOTF_GUARD(std::lock_guard(TheGraph()->get_graph_mutex()));
//        first_child = _create_child<LeafNodeRT>(this);
//        second_child = _create_child<LeafNodeRT>(this);
//        third_child = _create_child<LeafNodeRT>(this);
//    }
//    TypedNodeOwner<LeafNodeRT> first_child;
//    TypedNodeOwner<LeafNodeRT> second_child;
//    TypedNodeOwner<LeafNodeRT> third_child;
//};

//} // namespace

NOTF_CLOSE_NAMESPACE
