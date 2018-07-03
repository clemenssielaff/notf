#pragma once

#include "app/node.hpp"

NOTF_OPEN_NAMESPACE

namespace access { // forwards
template<class>
class _RootNode;
} // namespace access

// ================================================================================================================== //

/// The singular root node of a Scene hierarchy.
class RootNode : public Node {

    friend class access::_RootNode<Scene>;

    // types -------------------------------------------------------------------------------------------------------- //
public:
    /// Access types.
    template<class T>
    using Access = access::_RootNode<T>;

    // methods ------------------------------------------------------------------------------------------------------ //
private:
    NOTF_ALLOW_MAKE_SMART_FROM_PRIVATE;

    /// Constructor.
    /// @param token    Factory token provided by the Scene.
    /// @param scene    Scene to manage the RootNode.
    RootNode(FactoryToken token, Scene& scene) : Node(token, scene, this) {}

    /// Factory.
    /// Creates an unfinalized RootNode.
    /// @param scene    Scene to manage the RootNode.
    static RootNodePtr create(Scene& scene)
    {
        RootNodePtr result = NOTF_MAKE_SHARED_FROM_PRIVATE(RootNode, FactoryToken(), scene);
        s_unfinalized_nodes.emplace(result.get());
        return result;
    }

public:
    /// Destructor.
    ~RootNode() override;

    /// Sets a new child at the top of the Scene hierarchy (below the root).
    /// @param args Arguments that are forwarded to the constructor of the child.
    template<class T, class... Args>
    NodeHandle<T> set_child(Args&&... args)
    {
        _clear_children();
        return _add_child<T>(std::forward<Args>(args)...);
    }

    /// Removes the child of the root node, effectively clearing the Scene.
    void clear() { _clear_children(); }

private:
    /// Finalizes the RootNode.
    using Node::_finalize;
};

// accessors -------------------------------------------------------------------------------------------------------- //

template<>
class access::_RootNode<Scene> {
    friend class notf::Scene;

    /// Constructor.
    /// @param root_node    RootNode to operate on.
    _RootNode(RootNode& root_node) : m_root_node(root_node) {}

    /// Factory.
    /// @param scene    Scene to manage the RootNode.
    static RootNodePtr create(Scene& scene) { return RootNode::create(scene); }

    /// Finalizes the RootNode.
    void finalize() { m_root_node._finalize(); }

public:
    /// Constructs a new Property on the RootNode.
    /// @param name         Name of the Property.
    /// @param value        Initial value of the Property (also determines its type unless specified explicitly).
    /// @param validator    Optional validator function.
    /// @param has_body     Whether or not the Property will have a Property body in the Property Graph.
    /// @throws node_finalized_error
    ///                 If you call this method from anywhere but the constructor.
    /// @throws Path::not_unique_error
    ///                 If there already exists a Property of the same name on this Node.
    /// @throws NodeProperty::initial_value_error
    ///                 If the value of the Property could not be validated.
    template<class T>
    PropertyHandle<T>
    create_property(std::string name, T&& value, PropertyGraph::Validator<T> validator = {}, const bool has_body = true)
    {
        return m_root_node._create_property(std::move(name), std::forward<T>(value), std::move(validator), has_body);
    }

    // fields -----------------------------------------------------------------
private:
    /// RootNode to operate on.
    RootNode& m_root_node;
};

NOTF_CLOSE_NAMESPACE
