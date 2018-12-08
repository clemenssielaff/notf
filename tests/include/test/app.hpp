#pragma once

#include "notf/app/application.hpp"
#include "notf/app/node_runtime.hpp"
#include "notf/app/root_node.hpp"

#include "notf/reactive/trigger.hpp"

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

    using InternalFlags = Node::InternalFlags;

    Accessor(Node& node) : m_node(node) {}

    template<class NodeType>
    Accessor(TypedNodeHandle<NodeType>& node)
        : m_node(*(TypedNodeHandle<NodeType>::template AccessFor<Tester>::to_shared_ptr(node).get())) {}

    template<class NodeType>
    Accessor(TypedNodeOwner<NodeType>& node)
        : m_node(*(TypedNodeHandle<NodeType>::template AccessFor<Tester>::to_shared_ptr(node).get())) {}

    size_t get_property_hash() const { return m_node._calculate_property_hash(); }

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

    bool get_internal_flag(size_t index) { return m_node._get_internal_flag(index); }

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
    using InternalFlags = Node::AccessFor<Tester>::InternalFlags;

public:
    static constexpr auto to_none = detail::NoneSlot::name;
    static constexpr auto to_int = detail::IntSlot::name;
    static constexpr auto on_none = detail::NoneSignal::name;
    static constexpr auto on_int = detail::IntSignal::name;

public:
    TestNodeCT(valid_ptr<Node*> parent) : CompileTimeNode<detail::TestNodePolicy>(parent) {
        m_int_slot_pipe
            = make_pipeline(_get_slot<to_int>() | Trigger([this](const int& value) { m_int_slot_value = value; }));
    }

    template<class T, class... Args>
    auto create_child(Args... args) {
        return _create_child<T>(this, std::forward<Args>(args)...);
    }

    void set_parent(NodeHandle parent) { _set_parent(std::move(parent)); }
    bool get_flag(size_t index) const { return _get_flag(index); }
    void set_flag(size_t index, bool value = true) { _set_flag(index, value); }
    bool is_dirty() const {
        return Node::AccessFor<Tester>(*const_cast<TestNodeCT*>(this))
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

// run time test node =============================================================================================== //

class TestNodeRT : public RunTimeNode {
    using InternalFlags = Node::AccessFor<Tester>::InternalFlags;

public:
    TestNodeRT(valid_ptr<Node*> parent) : RunTimeNode(parent) {
        _create_property<float>("float", 0.123f, true);
        _create_property<bool>("bool", true, false);
        _create_property<int>("int", 123, true);

        _create_slot<None>("to_none");
        _create_slot<int>("to_int");

        _create_signal<None>("on_none");
        _create_signal<int>("on_int");

        m_int_slot_pipe
            = make_pipeline(_get_slot<int>("to_int") | Trigger([this](const int& value) { m_int_slot_value = value; }));
    }

    template<class T, class... Args>
    auto create_child(Args... args) {
        return _create_child<T>(this, std::forward<Args>(args)...);
    }

    void set_parent(NodeHandle parent) { _set_parent(std::move(parent)); }
    bool get_flag(size_t index) const { return _get_flag(index); }
    void set_flag(size_t index, bool value = true) { _set_flag(index, value); }
    bool is_dirty() const {
        return Node::AccessFor<Tester>(*const_cast<TestNodeRT*>(this))
            .get_internal_flag(to_number(InternalFlags::DIRTY));
    }
    template<class T>
    void emit(const std::string& name, const T& value) {
        _emit(name, value);
    }
    void emit(const std::string& name) { _emit(name); }
    int get_int_slot_value() const { return m_int_slot_value; }
    void fail_create_signal_finalized() { _create_signal<int>("already finalized"); }
    void fail_create_slot_finalized() { _create_slot<int>("already finalized"); }

public:
    int m_int_slot_value = 0;

private:
    AnyPipelinePtr m_int_slot_pipe;
};

// test node handle interface ======================================================================================= //

namespace detail {

template<>
struct NodeHandleInterface<TestNodeCT> : public NodeHandleBaseInterface<TestNodeCT> {
    using TestNodeCT::create_child;
    using TestNodeCT::emit;
    using TestNodeCT::get_flag;
    using TestNodeCT::get_int_slot_value;
    using TestNodeCT::is_dirty;
    using TestNodeCT::set_flag;
    using TestNodeCT::set_parent;
};

template<>
struct NodeHandleInterface<TestNodeRT> : public NodeHandleBaseInterface<TestNodeRT> {
    using TestNodeRT::create_child;
    using TestNodeRT::emit;
    using TestNodeRT::get_flag;
    using TestNodeRT::get_int_slot_value;
    using TestNodeRT::is_dirty;
    using TestNodeRT::set_flag;
    using TestNodeRT::set_parent;
    using TestNodeRT::fail_create_signal_finalized;
    using TestNodeRT::fail_create_slot_finalized;
};

} // namespace detail

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

NOTF_CLOSE_NAMESPACE
