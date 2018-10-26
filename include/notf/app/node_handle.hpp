#pragma once

#include "notf/meta/pointer.hpp"
#include "notf/meta/stringtype.hpp"

#include "notf/common/uuid.hpp"

#include "./app.hpp"

NOTF_OPEN_NAMESPACE

// node handle base ================================================================================================= //

namespace detail {

/// Base class of all NodeHandles.
/// Offers static functions that allow us to not include node.hpp in this header.
struct AnyNodeHandle {

    // methods --------------------------------------------------------------------------------- //
protected:
    /// The Uuid of this Node.
    static Uuid _get_uuid(NodePtr&& node);

    /// The Graph-unique name of this Node.
    static std::string _get_name(NodePtr&& node);

    /// (Re-)Names the Node.
    static void _set_name(NodePtr&& node, const std::string& name);

    static void _remove_from_parent(NodePtr&& node);
};

} // namespace detail

// typed node handle ================================================================================================ //

/// Members common to NodeHandle and NodeOwner.
template<class NodeType>
class TypedNodeHandle : public detail::AnyNodeHandle {

    static_assert(std::is_base_of_v<Node, NodeType>, "NodeType must be a type derived from Node");

    friend ::std::hash<TypedNodeHandle<NodeType>>;
    friend ::notf::Node;

    // types ----------------------------------------------------------------------------------- //
public:
    /// Nested `AccessFor<T>` type.
    NOTF_ACCESS_TYPE(TypedNodeHandle);

    /// Type of Node.
    using node_t = NodeType;

private:
    /// Short name of this type.
    using this_t = TypedNodeHandle;

    // methods --------------------------------------------------------------------------------- //
public:
    /// Default (empty) Constructor.
    TypedNodeHandle() = default;

    /// @{
    /// Value Constructor.
    /// @param  node    Node to handle.
    TypedNodeHandle(std::shared_ptr<NodeType> node) : m_node(std::move(node)) {}
    template<class T, class = std::enable_if_t<std::is_base_of_v<NodeType, T>>>
    TypedNodeHandle(std::shared_ptr<T> node) : m_node(std::static_pointer_cast<NodeType>(std::move(node)))
    {}
    /// @}

    /// @{
    /// Checks whether the NodeHandle is still valid or not.
    /// Note that there is a non-zero chance that a Handle is expired when you use it, even if `is_expired` just
    /// returned false, because it might have expired in the time between the test and the next call.
    /// However, if `is_expired` returns true, you can be certain that the Handle is expired for good.
    bool is_expired() const { return m_node.expired(); }
    explicit operator bool() const { return !is_expired(); }
    /// @}

    /// The Uuid of this Node.
    /// @throws     HandleExpiredError  If the Handle has expired.
    Uuid get_uuid() const { return _get_uuid(_get_node()); }

    /// The Graph-unique name of this Node.
    /// @returns    Name of this Node. Is an l-value because the name of the Node may change.
    /// @throws     HandleExpiredError  If the Handle has expired.
    std::string get_name() const { return _get_name(_get_node()); }

    /// (Re-)Names the Node.
    /// If another Node with the same name already exists in the Graph, this method will append the lowest integer
    /// postfix that makes the name unique.
    /// @param name     Proposed name of the Node.
    void set_name(const std::string& name) { _set_name(_get_node(), name); }

    template<class T>
    PropertyHandle<T> get_property(const std::string& name) const
    {
        return _get_node()->template get_property<T>(name);
    }

    /// Returns a correctly typed Handle to a CompileTimeProperty or void (which doesn't compile).
    /// @param name     Name of the requested Property.
    template<char... Cs, class X = NodeType, class = std::enable_if_t<detail::is_compile_time_node_v<X>>>
    auto get_property(StringType<char, Cs...> name) const
    {
        return this->_get_node()->get_property(std::forward<decltype(name)>(name));
    }

    /// Comparison with another NodeHandle.
    bool operator==(const this_t& rhs) const noexcept { return weak_ptr_equal(m_node, rhs.m_node); }
    bool operator!=(const this_t& rhs) const noexcept { return !operator==(rhs); }

    /// Less-than operator with another NodeHandle.
    bool operator<(const this_t& rhs) const noexcept { return m_node.owner_before(rhs.m_node); }

    /// Comparison with a NodePtr.
    friend bool operator==(const this_t& lhs, const NodePtr& rhs) noexcept { return lhs._get_ptr() == rhs.get(); }
    friend bool operator!=(const this_t& lhs, const NodePtr& rhs) noexcept { return !(lhs == rhs); }
    friend bool operator==(const NodePtr& lhs, const this_t& rhs) noexcept { return lhs.get() == rhs._get_ptr(); }
    friend bool operator!=(const NodePtr& lhs, const this_t& rhs) noexcept { return !(lhs == rhs); }

    /// Less-than operator with a NodePtr.
    friend bool operator<(const this_t& lhs, const NodePtr& rhs) noexcept { return lhs._get_ptr() < rhs.get(); }
    friend bool operator<(const NodePtr& lhs, const this_t& rhs) noexcept { return lhs.get() < rhs._get_ptr(); }

protected:
    /// Raw pointer to the handled Node (does not check if the Node is still alive).
    const Node* _get_ptr() const { return static_cast<const Node*>(raw_from_weak_ptr(m_node)); }

    /// Locks and returns an owning pointer to the handled Node.
    /// @throws HandleExpiredError  If the Handle has expired.
    std::shared_ptr<NodeType> _get_node() const
    {
        if (auto node = m_node.lock()) { return node; }
        NOTF_THROW(HandleExpiredError, "Node Handle has expired");
    }

    // fields ---------------------------------------------------------------------------------- //
protected:
    /// The handled Node, non owning.
    std::weak_ptr<NodeType> m_node;
};

// typed node owner ================================================================================================= //

/// Special NodeHandle type that is unique per Node instance and removes the Node when itself goes out of scope.
/// If the Node has already been removed by then, the destructor does nothing.
template<class NodeType>
class TypedNodeOwner : public TypedNodeHandle<NodeType> {

    friend struct ::std::hash<NodeOwner>;

    // methods --------------------------------------------------------------------------------- //
public:
    NOTF_NO_COPY_OR_ASSIGN(TypedNodeOwner);

    /// Default (empty) Constructor.
    TypedNodeOwner() = default;

    /// Value Constructor.
    /// @param node     Node to handle.
    /// @throws NotValidError   If the given Node pointer is empty.
    TypedNodeOwner(valid_ptr<std::shared_ptr<NodeType>> node) : TypedNodeHandle<NodeType>(node) {}

    /// Move assignment operator.
    TypedNodeOwner& operator=(TypedNodeOwner&& other) = default;

    /// Destructor.
    /// Note that the destruction of a Node requires the Graph mutex. Normally (if you store the Handle on the parent
    /// Node or some other Node in the Graph) this does not block, but it might if the mutex is not already held by this
    /// thread.
    ~TypedNodeOwner() { this->_remove_from_parent(this->m_node.lock()); }
};

NOTF_CLOSE_NAMESPACE

// std::hash specializations ======================================================================================== //

namespace std {

template<class NodeType>
struct hash<notf::TypedNodeHandle<NodeType>> {
    constexpr size_t operator()(const notf::TypedNodeHandle<NodeType>& handle) const noexcept
    {
        return notf::hash_mix(notf::to_number(handle._get_ptr()));
    }
};

} // namespace std
