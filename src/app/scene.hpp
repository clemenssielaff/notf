#pragma once

#include <unordered_map>

#include "app/node_container.hpp"
#include "app/scene_graph.hpp"
#include "common/size2.hpp"

NOTF_OPEN_NAMESPACE

namespace access {
template<class>
class _Scene;
template<class>
class _RootNode;
} // namespace access

// ================================================================================================================== //

class Scene {

    friend class access::_Scene<SceneGraph>;
    friend class access::_Scene<Node>;
#ifdef NOTF_TEST
    friend class access::_Scene<test::Harness>;
#endif

    // types -------------------------------------------------------------------------------------------------------- //
public:
    /// Access types.
    template<class T>
    using Access = access::_Scene<T>;

    /// Exception thrown when the name of a Scene is not unique within its SceneGraph.
    NOTF_EXCEPTION_TYPE(scene_name_error);

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
    Scene(FactoryToken, const valid_ptr<SceneGraphPtr>& graph, std::string name);

    /// Scene Factory method.
    /// @param graph    SceneGraph containing the Scene.
    /// @param name     Graph-unique, immutable name of the Scene.
    /// @param args     Additional arguments for the Scene subclass
    /// @throws scene_name_error    If the given name is not unique in the SceneGraph.
    template<class T, class... Args, typename = std::enable_if_t<std::is_base_of<Scene, T>::value>>
    static std::shared_ptr<T> create(const valid_ptr<SceneGraphPtr>& graph, std::string name, Args... args)
    {
        NOTF_GUARD(std::lock_guard(SceneGraph::Access<Scene>::event_mutex(*graph.get())));
        NOTF_GUARD(std::lock_guard(SceneGraph::Access<Scene>::hierarchy_mutex(*graph.get())));
        std::shared_ptr<T> scene
            = std::make_shared<T>(FactoryToken(), graph, std::move(name), std::forward<Args>(args)...);
        scene->_finalize_root();
        access::_SceneGraph<Scene>::register_scene(*graph, scene);
        return scene;
    }

    /// Destructor.
    virtual ~Scene();

    /// The SceneGraph owning this Scene.
    SceneGraph& get_graph() const { return *m_graph; }

    /// Window owning this Scene. Is empty if the Window was already closed.
    risky_ptr<WindowPtr> get_window() const { return m_graph->get_window(); }

    /// Graph-unique name of the Scene.
    const std::string& get_name() const { return m_name->first; }

    /// @{
    /// Searches for and returns a Property of a Node or the Scene.
    /// @param path     Path uniquely identifying a Property.
    /// @returns        Handle to the requested NodeProperty.
    ///                 Is empty if the Property doesn't exist or is of the wrong type.
    /// @throws Path::path_error    If the Path is invalid.
    template<class T>
    PropertyHandle<T> get_property(const Path& path)
    {
        return PropertyHandle<T>(std::dynamic_pointer_cast<TypedNodeProperty<T>>(_get_property(path)));
    }
    template<class T>
    PropertyHandle<T> get_property(const std::string& path)
    {
        return get_property<T>(Path(path));
    }
    /// @}

    /// @{
    /// Searches for and returns a Node in this Scene.
    /// @param path     Path uniquely identifying a Node.
    /// @returns        Handle to the requested Node.
    ///                 Is empty if the Node doesn't exist or is of the wrong type.
    /// @throws Path::path_error    If the Path is invalid.
    template<class T = Node>
    NodeHandle<T> get_node(const Path& path)
    {
        return NodeHandle<T>(std::dynamic_pointer_cast<T>(_get_node(path)));
    }
    template<class T = Node>
    NodeHandle<T> get_node(const std::string& path)
    {
        return get_node<T>(Path(path));
    }
    /// @}

    /// The number of Nodes in the Scene including the root node (therefore is always >= 1).
    size_t count_nodes() const;

    /// Removes all nodes (except the root node) from the Scene.
    void clear();

protected:
    /// The unique root node of this Scene.
    RootNode& _get_root() const { return *m_root; }

    /// Special access to this Scene's RootNode.
    access::_RootNode<Scene> _get_root_access();

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
    risky_ptr<NodeContainer*> _get_frozen_children(valid_ptr<const Node*> node);

    /// Creates a frozen child container for the given node.
    /// @param node             Node to freeze.
    void _create_frozen_children(valid_ptr<const Node*> node);

    /// Private and untyped implementation of `property` assuming sanitized inputs.
    /// @param path     Path uniquely identifying a Property.
    /// @returns        Handle to the requested NodeProperty. Is empty if the Property doesn't exist.
    /// @throws Path::path_error    If the Path does not lead to a Property.
    NodePropertyPtr _get_property(const Path& path);

    /// Private and untyped implementation of `node` assuming sanitized inputs.
    /// @param path     Path uniquely identifying a Node.
    /// @returns        Handle to the requested Node. Is empty if the Node doesn't exist.
    /// @throws Path::path_error    If the Path does not lead to a Node.
    NodePtr _get_node(const Path& path);

    /// Finalizes the RootNode of this Scene after creation.
    void _finalize_root();

    // fields ------------------------------------------------------------------------------------------------------- //
private:
    /// The SceneGraph owning this Scene.
    SceneGraphPtr m_graph;

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
    /// @returns        The requested NodeProperty, is empty is none exists.
    /// @throws Path::path_error    If the Path is invalid.
    static NodePropertyPtr get_property(Scene& scene, const Path& path) { return scene._get_property(path); }

    /// Searches for and returns a Node in the Scene.
    /// @param path     Path uniquely identifying a Node.
    /// @returns        The requested Node, is empty is none exists.
    /// @throws Path::path_error    If the Path is invalid.
    static NodePtr get_node(Scene& scene, const Path& path) { return scene._get_node(path); }
};

//-----------------------------------------------------------------------------

template<>
class access::_Scene<Node> {
    friend class notf::Node;

    /// Finds a delta for the given child container and returns it, if it exists.
    /// @param scene            Scene to operate on.
    /// @param node             Node whose delta to return.
    static risky_ptr<NodeContainer*> get_delta(Scene& scene, valid_ptr<const Node*> node)
    {
        return scene._get_frozen_children(node);
    }

    /// Creates a new delta for the given child container.
    /// @param scene            Scene to operate on.
    /// @param node             Node whose delta to create a delta for.
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
