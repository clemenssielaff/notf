#pragma once

#include <utility>

#include "./property.hpp"

NOTF_OPEN_NAMESPACE

// node ============================================================================================================= //

class Node : public std::enable_shared_from_this<Node> {

    // type --------------------------------------------------------------------------------------------------------- //
public:
    /// Exception thrown by `get_property` if either the name of the type of the requested Property is wrong.
    NOTF_EXCEPTION_TYPE(NoSuchPropertyError);

    // methods ------------------------------------------------------------------------------------------------------ //
public:
    NOTF_NO_COPY_OR_ASSIGN(Node);

    /// Value constructor.
    /// @param parent   Parent of this Node.
    Node(valid_ptr<Node*> parent);

    /// Destructor.
    virtual ~Node();

    /// Uuid of this Node.
    const Uuid& get_uuid() const { return m_uuid; }

    /// Run time access to a Property of this Node.
    template<class T>
    PropertyHandle<T> get_property(std::string_view name) const
    {
        AnyPropertyPtr& property = _get_property(name);
        if (!property) {
            NOTF_THROW(NoSuchPropertyError, "Node \"{}\" has no Property called \"{}\"", get_name(), name);
        }

        PropertyPtr<T> typed_property = std::dynamic_pointer_cast<Property<T>>(property);
        if (!typed_property) {
            NOTF_THROW(NoSuchPropertyError,
                       "Property \"{}\" of Node \"{}\" is of type \"{}\", but was requested as \"{}\"", //
                       name, get_name(), property->get_type_name(), type_name<T>()());
        }
        return PropertyHandle(std::move(typed_property));
    }

    std::string_view get_name() const { return "Node Name Here"; } // TODO: random node names

protected:
    virtual AnyPropertyPtr& _get_property(std::string_view name) const = 0;

    // fields ------------------------------------------------------------------------------------------------------- //
private:
    /// Uuid of this Node.
    const Uuid m_uuid = Uuid::generate();

    /// Parent of this Node.
    valid_ptr<Node*> m_parent;

    /// Name of this Node. Is empty until requested for the first time or set using `set_name`.
    std::string_view m_name;
};

// node handle ====================================================================================================== //

class NodeHandle {

    // methods ------------------------------------------------------------------------------------------------------ //
public:
    /// Default (empty) Constructor.
    NodeHandle() = default;

    /// @{
    /// Value Constructor.
    /// @param  node    Node to handle.
    NodeHandle(const NodePtr& node) : m_node(node) {}
    NodeHandle(NodeWeakPtr node) : m_node(std::move(node)) {}
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
    /// @throws HandleExpiredError  If the Handle has expired.
    const Uuid& get_uuid() const { return _node()->get_uuid(); }

    /// The Node-unique name of this Node.
    /// @throws HandleExpiredError  If the Handle has expired.
    std::string_view get_name() const { return _node()->get_name(); }

    /// Comparison operator.
    /// @param rhs  Other NodeHandle to compare against.
    bool operator==(const NodeHandle& rhs) const noexcept { return weak_ptr_equal(m_node, rhs.m_node); }
    bool operator!=(const NodeHandle& rhs) const noexcept { return !operator==(rhs); }

    /// Less-than operator.
    /// @param rhs  Other NodeHandle to compare against.
    bool operator<(const NodeHandle& rhs) const noexcept { return m_node.owner_before(rhs.m_node); }

private:
    /// Locks and returns an owning pointer to the handled Node.
    /// @throws HandleExpiredError  If the Handle has expired.
    NodePtr _node() const
    {
        if (auto node = m_node.lock()) { return node; }
        NOTF_THROW(HandleExpiredError, "Node Handle has expired");
    }

    // members ------------------------------------------------------------------------------------------------------ //
private:
    /// The handled Node, non owning.
    NodeWeakPtr m_node;
};

NOTF_CLOSE_NAMESPACE
