#pragma once

#include "notf/app/application.hpp"
#include "notf/app/graph/root_node.hpp"

#include "notf/reactive/trigger.hpp"

NOTF_OPEN_NAMESPACE

// accessors ======================================================================================================== //

/// Node Handle
template<class NodeType>
struct Accessor<NodeHandle<NodeType>, Tester> {
    static std::shared_ptr<NodeType> to_shared_ptr(const NodeHandle<NodeType>& handle) {
        return handle.m_node.lock();
    }
};
template<class NodeType>
auto to_shared_ptr(NodeHandle<NodeType> node) {
    return NodeHandle<NodeType>::template AccessFor<Tester>::to_shared_ptr(node);
}

/// Node
template<>
struct Accessor<AnyNode, Tester> {

    using InternalFlags = AnyNode::InternalFlags;

    Accessor(AnyNode& node) : m_node(node) {}

    template<class NodeType>
    Accessor(NodeHandle<NodeType>& node)
        : m_node(*(NodeHandle<NodeType>::template AccessFor<Tester>::to_shared_ptr(node).get())) {}

    template<class NodeType>
    Accessor(NodeOwner<NodeType>& node)
        : m_node(*(NodeHandle<NodeType>::template AccessFor<Tester>::to_shared_ptr(node).get())) {}

    size_t get_property_hash() const { return m_node._calculate_property_hash(); }

    void set_uuid(const Uuid& uuid) { const_cast<Uuid&>(m_node.m_uuid) = uuid; }

    bool has_ancestor(const AnyNode* node) const { return m_node._has_ancestor(node); }

    static constexpr size_t get_user_flag_count() noexcept { return AnyNode::s_user_flag_count; }

    //    bool get_flag(const size_t index, const std::thread::id thread_id = std::this_thread::get_id()) const {
    //        return m_node._get_internal_flag(index + Node::s_internal_flag_count, thread_id);
    //    }

    template<class T, class... Args>
    auto create_child(Args... args) {
        return m_node._create_child<T>(&m_node, std::forward<Args>(args)...);
    }

    void remove_child(AnyNodeHandle handle) { m_node._remove_child(handle); }

    bool get_internal_flag(size_t index) { return m_node._get_internal_flag(index); }

    //    void set_parent(AnyNodeHandle parent) {
    //        NOTF_GUARD(std::lock_guard(TheGraph()->get_graph_mutex()));
    //        m_node._set_parent(parent);
    //    }

    //    AnyNodeHandle get_parent(std::thread::id thread_id = std::this_thread::get_id()) {
    //        Node* parent = m_node._get_parent(thread_id);
    //        if (parent) { return parent->shared_from_this(); }
    //        return {};
    //    }

    //    size_t get_child_count(std::thread::id thread_id = std::this_thread::get_id()) {
    //        NOTF_GUARD(std::lock_guard(TheGraph()->get_graph_mutex()));
    //        return m_node._read_children(thread_id).size();
    //    }

    AnyNode& m_node;
};

/// Graph
template<>
struct Accessor<detail::Graph, Tester> {
    //    static detail::Graph& get() { return *TheGraph(); }
    static auto register_node(AnyNodeHandle node) { return TheGraph()->m_node_registry.add(to_shared_ptr(node)); }
};

// empty node ========================================================================================================
// //

using EmptyNode = Node<detail::EmptyNodePolicy>;

// test node ======================================================================================================== //

namespace detail {

// properties =================================================================

struct FloatPropertyPolicy {
    using value_t = float;
    static constexpr ConstString name = "float";
    static constexpr value_t default_value = 0.123f;
    static constexpr bool is_visible = true;
};

struct BoolPropertyPolicy {
    using value_t = bool;
    static constexpr ConstString name = "bool";
    static constexpr value_t default_value = true;
    static constexpr bool is_visible = false;
};

struct IntPropertyPolicy {
    using value_t = int;
    static constexpr ConstString name = "int";
    static constexpr value_t default_value = 123;
    static constexpr bool is_visible = true;
};

// slots ======================================================================

struct NoneSlot {
    using value_t = None;
    static constexpr ConstString name = "to_none";
};

struct IntSlot {
    using value_t = int;
    static constexpr ConstString name = "to_int";
};

// signals ====================================================================

struct NoneSignal {
    using value_t = None;
    static constexpr ConstString name = "on_none";
};

struct IntSignal {
    using value_t = int;
    static constexpr ConstString name = "on_int";
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

class TestNode : public Node<detail::TestNodePolicy> {
    using InternalFlags = AnyNode::AccessFor<Tester>::InternalFlags;

public:
    static constexpr auto to_none = detail::NoneSlot::name;
    static constexpr auto to_int = detail::IntSlot::name;
    static constexpr auto on_none = detail::NoneSignal::name;
    static constexpr auto on_int = detail::IntSignal::name;

public:
    TestNode(valid_ptr<AnyNode*> parent) : Node<detail::TestNodePolicy>(parent) {
        m_int_slot_pipe
            = make_pipeline(_get_slot<to_int>() | Trigger([this](const int& value) { m_int_slot_value = value; }));
    }

    template<class T, class... Args>
    auto create_child(Args... args) {
        return _create_child<T>(this, std::forward<Args>(args)...);
    }

    void set_parent(AnyNodeHandle parent) { _set_parent(std::move(parent)); }
    bool get_flag(size_t index) const { return _get_flag(index); }
    void set_flag(size_t index, bool value = true) { _set_flag(index, value); }
    bool is_dirty() const {
        return AnyNode::AccessFor<Tester>(*const_cast<TestNode*>(this))
            .get_internal_flag(to_number(InternalFlags::DIRTY));
    }
    template<class T>
    void emit(const std::string& name, const T& value) {
        _emit(name, value);
    }
    void emit(const std::string& name) { _emit(name); }
    int get_int_slot_value() const { return m_int_slot_value; }

public:
    int m_int_slot_value = 0;

private:
    AnyPipelinePtr m_int_slot_pipe;
};

// test node handle interface ======================================================================================= //

namespace detail {

template<>
struct NodeHandleInterface<TestNode> : public NodeHandleBaseInterface<TestNode> {
    using TestNode::create_child;
    using TestNode::emit;
    using TestNode::get_flag;
    using TestNode::get_int_slot_value;
    using TestNode::is_dirty;
    using TestNode::set_flag;
    using TestNode::set_parent;
};

} // namespace detail

// functions ======================================================================================================== //

struct TheRootNode {

    TheRootNode() : m_root(NodeHandle<RootNode>::AccessFor<Tester>::to_shared_ptr(TheGraph()->get_root_node())) {}

    template<class T, class... Args>
    auto create_child(Args... args) {
        return AnyNode::AccessFor<Tester>(*m_root).create_child<T>(std::forward<Args>(args)...);
    }

private:
    std::shared_ptr<RootNode> m_root;
};

NOTF_CLOSE_NAMESPACE
