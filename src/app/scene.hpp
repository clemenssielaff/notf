#pragma once

#include <unordered_map>

#include "app/node_container.hpp"
#include "app/scene_graph.hpp"
#include "common/size2.hpp"

NOTF_OPEN_NAMESPACE

namespace access { // forwards
template<class>
class _Scene;
class _Scene_NodeHandle;
} // namespace access

// ================================================================================================================== //

class Scene {
    friend class access::_Scene<SceneGraph>;
    friend class access::_Scene<Node>;
    friend class access::_Scene_NodeHandle;
#ifdef NOTF_TEST
    friend class access::_Scene<test::Harness>;
#endif

    // types -------------------------------------------------------------------------------------------------------- //
public:
    /// Access types.
    template<class T>
    using Access = access::_Scene<T>;
    using NodeHandleAccess = access::_Scene_NodeHandle;

    /// Exception thrown when the name of a Scene is not unique within its SceneGraph.
    NOTF_EXCEPTION_TYPE(scene_name_error);

    /// Exception thrown when the SceneGraph has gone out of scope before a Scene tries to access it.
    NOTF_EXCEPTION_TYPE(no_graph_error);

    /// Thrown when a node did not have the expected position in the hierarchy.
    NOTF_EXCEPTION_TYPE(hierarchy_error);

protected:
    /// Factory token object to make sure that Scene instances can only be created by a call to `create`.
    class FactoryToken {
        friend class Scene;
        FactoryToken() {} // not `= default`, otherwise you could construct a Token via `Token{}`.
    };

    // methods ------------------------------------------------------------------------------------------------------ //
public:
    NOTF_NO_COPY_OR_ASSIGN(Scene);

    /// Constructor.
    /// @param token    Factory token provided by Scene::create.
    /// @param graph    The SceneGraph owning this Scene.
    /// @param name     Graph-unique, immutable name of the Scene.
    /// @throws scene_name_error    If the given name is not unique in the SceneGraph.
    Scene(const FactoryToken&, const valid_ptr<SceneGraphPtr>& graph, std::string name);

    /// Scene Factory method.
    /// @param graph    SceneGraph containing the Scene.
    /// @param name     Graph-unique, immutable name of the Scene.
    /// @param args     Additional arguments for the Scene subclass
    /// @throws scene_name_error    If the given name is not unique in the SceneGraph.
    template<class T, class... Args, typename = std::enable_if_t<std::is_base_of<Scene, T>::value>>
    static std::shared_ptr<T> create(const valid_ptr<SceneGraphPtr>& graph, std::string name, Args... args)
    {
        NOTF_MUTEX_GUARD(SceneGraph::Access<Scene>::mutex(*graph.get()));
        std::shared_ptr<T> scene
            = std::make_shared<T>(FactoryToken(), graph, std::move(name), std::forward<Args>(args)...);
        access::_SceneGraph<Scene>::register_scene(*graph, scene);
        return scene;
    }

    /// Destructor.
    virtual ~Scene();

    /// @{
    /// The SceneGraph owning this Scene.
    /// @throws no_graph_error  If the SceneGraph of the node has been deleted.
    valid_ptr<SceneGraphPtr> graph()
    {
        SceneGraphPtr scene_graph = m_graph.lock();
        if (NOTF_UNLIKELY(!scene_graph)) {
            notf_throw(no_graph_error, "SceneGraph has been deleted");
        }
        return scene_graph;
    }
    valid_ptr<SceneGraphConstPtr> graph() const { return const_cast<Scene*>(this)->graph(); }
    /// @}

    /// Graph-unique name of the Scene.
    const std::string& name() const { return m_name->first; }

    /// @{
    /// The unique root node of this Scene.
    RootNode& root() { return *m_root; }
    const RootNode& root() const { return *m_root; }
    /// @}

    /// Searches for and returns a Property of a Node or the Scene.
    /// @param path     Path uniquely identifying a Property.
    /// @throws Path::path_error    If the Path does not lead to a Property.
    PropertyReader property(const Path& path) const;

    /// The number of Nodes in the Scene including the root node (therefore is always >= 1).
    size_t count_nodes() const;

    /// Removes all nodes (except the root node) from the Scene.
    void clear();

    // event handling ---------------------------------------------------------
private:
    /// Handles an untyped event.
    /// @returns True iff the event was handled.
    virtual bool _handle_event(Event& event NOTF_UNUSED) { return false; }

    /// Resizes the view on this Scene.
    /// @param size Size of the view in pixels.
    virtual void _resize_view(Size2i size) = 0;

    /// Called by the SceneGraph after unfreezing, resolves all deltas in this Scene.
    void _clear_delta();

    /// Checks if the given Scene name is unique in the given SceneGraph.
    /// @param graph    SceneGraph of this Scene.
    /// @param name     Scene name to test.
    /// @returns        Iterator to the name reserved in the SceneGraph.
    /// @throws scene_name_error    If the given name is not unique in the SceneGraph.
    static SceneGraph::SceneMap::const_iterator _validate_scene_name(SceneGraph& graph, std::string name);

    // scene hierarchy --------------------------------------------------------
private:
    /// Finds and returns the frozen child container for a given node, if one exists.
    /// @param node             Frozen Node.
    /// @throws no_graph_error  If the SceneGraph of the node has been deleted.
    risky_ptr<NodeContainer*> _get_frozen_children(valid_ptr<const Node*> node);

    /// Creates a frozen child container for the given node.
    /// @param node             Node to freeze.
    /// @throws no_graph_error  If the SceneGraph of the node has been deleted.
    void _create_frozen_children(valid_ptr<const Node*> node);

    /// Private implementation of `property` assuming sanitized inputs.
    /// @param path     Path uniquely identifying a Property.
    /// @throws Path::path_error    If the Path does not lead to a Property.
    PropertyReader _property(const Path& path) const;

    // fields ------------------------------------------------------------------------------------------------------- //
private:
    /// The SceneGraph owning this Scene.
    SceneGraphWeakPtr m_graph;

    /// Graph-unique, immutable name of the Scene.
    const SceneGraph::SceneMap::const_iterator m_name;

    /// Map containing copieds of ChildContainer that were modified while the SceneGraph was frozen.
    std::unordered_map<const Node*, NodeContainer, pointer_hash<const Node*>> m_frozen_children;

    /// Set of Nodes containing one or more Properties that were modified while the SceneGraph was frozen.
    std::unordered_set<valid_ptr<NodePtr>, pointer_hash<valid_ptr<NodePtr>>> m_tweaked_nodes;

    /// The singular root node of the scene hierarchy.
    RootNodePtr m_root;
};

// accessors -------------------------------------------------------------------------------------------------------- //

template<>
class access::_Scene<SceneGraph> {
    friend class notf::SceneGraph;

    /// Handles an untyped event.
    /// @param scene    Scene to operate on.
    /// @returns        True iff the event was handled.
    static bool handle_event(Scene& scene, Event& event) { return scene._handle_event(event); }

    /// Resizes the view on this Scene.
    /// @param scene    Scene to operate on.
    /// @param size     Size of the view in pixels.
    static void resize_view(Scene& scene, Size2i size) { scene._resize_view(std::move(size)); }

    /// Called by the SceneGraph after unfreezing, resolves all deltas in this Scene.
    /// @param scene    Scene to operate on.
    static void clear_delta(Scene& scene) { scene._clear_delta(); }

    /// Searches for and returns a Property of a Node or the Scene.
    /// @param path     Path uniquely identifying a Property.
    /// @throws Path::path_error    If the Path does not lead to a Property.
    static PropertyReader property(const Scene& scene, const Path& path) { return scene._property(path); }
};

//-----------------------------------------------------------------------------

template<>
class access::_Scene<Node> {
    friend class notf::Node;

    /// Finds a delta for the given child container and returns it, if it exists.
    /// @param scene            Scene to operate on.
    /// @param node             Node whose delta to return.
    /// @throws no_graph_error  If the SceneGraph of the node has been deleted.
    static risky_ptr<NodeContainer*> get_delta(Scene& scene, valid_ptr<const Node*> node)
    {
        return scene._get_frozen_children(node);
    }

    /// Creates a new delta for the given child container.
    /// @param scene            Scene to operate on.
    /// @param node             Node whose delta to create a delta for.
    /// @throws no_graph_error  If the SceneGraph of the node has been deleted.
    static void create_delta(Scene& scene, valid_ptr<const Node*> node) { scene._create_frozen_children(node); }

    /// Registers a Node as being "tweaked".
    /// A Node is tweaked when it has one or more Properties that were modified while the SceneGraph was frozen.
    /// @param scene            Scene to operate on.
    /// @param node             Node to register as tweaked.
    static void register_tweaked(Scene& scene, valid_ptr<NodePtr> node)
    {
        scene.m_tweaked_nodes.emplace(std::move(node));
    }
};

NOTF_CLOSE_NAMESPACE
