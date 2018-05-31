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
    RootNode(const FactoryToken& token, Scene& scene) : Node(token, scene, this) {}

    /// Factory.
    /// @param scene    Scene to manage the RootNode.
    static RootNodePtr create(Scene& scene) { return NOTF_MAKE_SHARED_FROM_PRIVATE(RootNode, FactoryToken(), scene); }

public:
    /// Destructor.
    ~RootNode() override;

    /// Sets  a new child at the top of the Scene hierarchy (below the root).
    /// @param args Arguments that are forwarded to the constructor of the child.
    /// @throws no_graph_error  If the SceneGraph of the node has been deleted.
    template<class T, class... Args>
    NodeHandle<T> set_child(Args&&... args)
    {
        _clear_children();
        return _add_child<T>(std::forward<Args>(args)...);
    }

    /// Removes the child of the root node, effectively clearing the Scene.
    /// @throws no_graph_error  If the SceneGraph of the node has been deleted.
    void clear() { _clear_children(); }
};

// accessors -------------------------------------------------------------------------------------------------------- //

template<>
class access::_RootNode<Scene> {
    friend class notf::Scene;

    /// Factory.
    /// @param scene    Scene to manage the RootNode.
    static RootNodePtr create(Scene& scene) { return RootNode::create(scene); }
};

NOTF_CLOSE_NAMESPACE
