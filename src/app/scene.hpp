#pragma once

#include <map>
#include <unordered_map>

#include "app/scene_graph.hpp"
#include "common/size2.hpp"
#include "common/string_view.hpp"

NOTF_OPEN_NAMESPACE

namespace access { // forwards
template<class>
class _Scene;
class _Scene_SceneNodeHandle;
} // namespace access

// ===================================================================================================================//

class Scene {
    friend class access::_Scene<SceneGraph>;
    friend class access::_Scene<SceneNode>;
    friend class access::_Scene_SceneNodeHandle;
#ifdef NOTF_TEST
    friend class access::_Scene<test::Harness>;
#endif

    // types ---------------------------------------------------------------------------------------------------------//
public:
    /// Access types.
    template<class T>
    using Access = access::_Scene<T>;
    using SceneNodeHandleAccess = access::_Scene_SceneNodeHandle;

    /// Exception thrown when the SceneGraph has gone out of scope before a Scene tries to access it.
    NOTF_EXCEPTION_TYPE(no_graph_error);

    /// Thrown when a node did not have the expected position in the hierarchy.
    NOTF_EXCEPTION_TYPE(hierarchy_error);

protected:
    /// Factory token object to make sure that Node instances can only be created by a call to `create`.
    class FactoryToken {
        friend class Scene;
        FactoryToken() {} // not `= default`, otherwise you could construct a Token via `Token{}`.
    };

private:
    /// Shared pointer to the root node.
    using RootPtr = valid_ptr<std::shared_ptr<RootSceneNode>>;

    // ------------------------------------------------------------------------

    /// Container for all child nodes of a SceneNode.
    class NodeContainer {

        friend class access::_Scene<SceneNode>;

        // methods ------------------------------------------------------------
    public:
        /// Tests if this container is empty.
        bool empty() const noexcept { return m_order.empty(); }

        /// Number of SceneNodes in the container.
        size_t size() const noexcept { return m_order.size(); }

        /// Checks if the container contains a SceneNode by the given name.
        /// @param name Name to check for.
        bool contains(const std::string& name) const { return m_names.count(name); }

        /// Checks if the container contains a given SceneNode.
        /// @param node SceneNode to check for
        bool contains(const SceneNode* node) const
        {
            for (const auto& child : m_order) {
                if (child.raw().get() == node) {
                    return true;
                }
            }
            return false;
        }

        /// Returns all names of nodes in this Container.
        std::set<std::string_view> all_names() const
        {
            std::set<std::string_view> result;
            for (const auto& it : m_names) {
                result.insert(it.first);
            }
            return result;
        }

        /// Adds a new SceneNode to the container.
        /// @param node Node to add.
        /// @returns    True iff the node was inserted successfully, false otherwise.
        bool add(valid_ptr<SceneNodePtr> node);

        /// Erases a given SceneNode from the container.
        void erase(const SceneNodePtr& node);

        /// Clears all SceneNodes from the container.
        void clear()
        {
            m_order.clear();
            m_names.clear();
        }

        /// Moves the given node in front of all of its siblings.
        /// @param node     Node to move.
        void stack_front(const valid_ptr<SceneNode*> node);

        /// Moves the given node behind all of its siblings.
        /// @param node     Node to move.
        void stack_back(const valid_ptr<SceneNode*> node);

        /// Moves the node at a given index before a given sibling.
        /// @param index    Index of the node to move
        /// @param sibling  Sibling to stack before.
        /// @throws hierarchy_error     If the sibling is not a sibling of this node.
        void stack_before(const size_t index, const valid_ptr<SceneNode*> relative);

        /// Moves the node at a given index behind a given sibling.
        /// @param index    Index of the node to move
        /// @param sibling  Sibling to stack behind.
        /// @throws hierarchy_error     If the sibling is not a sibling of this node.
        void stack_behind(const size_t index, const valid_ptr<SceneNode*> sibling);

        /// @{
        /// Reference to the SceneNode in the back of the stack.
        auto& back() { return m_order.front(); }
        const auto& back() const { return m_order.front(); }
        /// @}

        /// @{
        /// Reference to the SceneNode in the front of the stack.
        auto& front() { return m_order.back(); }
        const auto& front() const { return m_order.back(); }
        /// @}

        /// @{
        /// Iterator-adapters for traversing the contained SceneNodes in order.
        auto begin() noexcept { return m_order.begin(); }
        auto begin() const noexcept { return m_order.cbegin(); }
        auto cbegin() const noexcept { return m_order.cbegin(); }
        auto end() noexcept { return m_order.end(); }
        auto end() const noexcept { return m_order.cend(); }
        auto cend() const noexcept { return m_order.cend(); }
        /// @}

        /// @{
        /// Index-based access to a SceneNode in the container.
        auto& operator[](size_t pos) { return m_order[pos]; }
        const auto& operator[](size_t pos) const { return m_order[pos]; }
        /// @}

    private:
        /// Updates the name of one of the child nodes.
        /// This function DOES NOT UPDATE THE NAME OF THE NODE itself, just the name by which the parent knows it.
        /// @param node         Node to rename, must be part of the container and still have its old name.
        /// @param new_name     New name of the node.
        void _rename(valid_ptr<const SceneNode*> node, std::string new_name);

        // fields -------------------------------------------------------------
    private:
        /// All SceneNodes in order, also provides ownership.
        std::vector<valid_ptr<SceneNodePtr>> m_order;

        /// Provides name-based lookup of the contained SceneNodes.
        std::map<std::string, std::weak_ptr<SceneNode>> m_names;
    };

    // methods -------------------------------------------------------------------------------------------------------//
public:
    NOTF_NO_COPY_OR_ASSIGN(Scene);

    /// Constructor.
    /// @param token    Factory token provided by Scene::create.
    /// @param manager  The SceneGraph owning this Scene.
    Scene(const FactoryToken&, const valid_ptr<SceneGraphPtr>& graph);

    /// Scene Factory method.
    /// @param graph    SceneGraph containing the Scene.
    /// @param args     Additional arguments for the Scene subclass
    template<class T, class... Args, typename = std::enable_if_t<std::is_base_of<Scene, T>::value>>
    static std::shared_ptr<T> create(const valid_ptr<SceneGraphPtr>& graph, Args... args)
    {
        std::shared_ptr<T> scene = std::make_shared<T>(FactoryToken(), graph, std::forward<Args>(args)...);
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

    /// @{
    /// The unique root node of this Scene.
    RootSceneNode& root() { return *m_root; }
    const RootSceneNode& root() const { return *m_root; }
    /// @}

    /// The number of SceneNodes in the Scene including the root node (therefore is always >= 1).
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

    // scene hierarchy --------------------------------------------------------
private:
    /// Finds and returns the frozen child container for a given node, if one exists.
    /// @param node             Frozen SceneNode.
    /// @throws no_graph_error  If the SceneGraph of the node has been deleted.
    risky_ptr<NodeContainer*> _get_frozen_children(valid_ptr<const SceneNode*> node);

    /// Creates a frozen child container for the given node.
    /// @param node             SceneNode to freeze.
    /// @throws no_graph_error  If the SceneGraph of the node has been deleted.
    void _create_frozen_children(valid_ptr<const SceneNode*> node);

    // fields --------------------------------------------------------------------------------------------------------//
private:
    /// The SceneGraph owning this Scene.
    std::weak_ptr<SceneGraph> m_graph;

    /// Map containing copieds of ChildContainer that were modified while the SceneGraph was frozen.
    std::unordered_map<const SceneNode*, NodeContainer, pointer_hash<const SceneNode*>> m_frozen_children;

    /// Set of SceneNodes containing one or more Properties that were modified while the SceneGraph was frozen.
    std::unordered_set<valid_ptr<SceneNodePtr>, pointer_hash<valid_ptr<SceneNodePtr>>> m_tweaked_nodes;

    /// The singular root node of the scene hierarchy.
    RootPtr m_root;
};

// accessors ---------------------------------------------------------------------------------------------------------//

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
};

//-----------------------------------------------------------------------------

template<>
class access::_Scene<SceneNode> {
    friend class notf::SceneNode;

    /// Container used to store the children of a SceneNode.
    using NodeContainer = Scene::NodeContainer;

    /// Finds a delta for the given child container and returns it, if it exists.
    /// @param scene            Scene to operate on.
    /// @param node             SceneNode whose delta to return.
    /// @throws no_graph_error  If the SceneGraph of the node has been deleted.
    static risky_ptr<NodeContainer*> get_delta(Scene& scene, valid_ptr<const SceneNode*> node)
    {
        return scene._get_frozen_children(node);
    }

    /// Creates a new delta for the given child container.
    /// @param scene            Scene to operate on.
    /// @param node             SceneNode whose delta to create a delta for.
    /// @throws no_graph_error  If the SceneGraph of the node has been deleted.
    static void create_delta(Scene& scene, valid_ptr<const SceneNode*> node) { scene._create_frozen_children(node); }

    /// Registers a SceneNode as being "tweaked".
    /// A SceneNode is tweaked when it has one or more Properties that were modified while the SceneGraph was frozen.
    /// @param scene            Scene to operate on.
    /// @param node             Node to register as tweaked.
    static void register_tweaked(Scene& scene, valid_ptr<SceneNodePtr> node)
    {
        scene.m_tweaked_nodes.emplace(std::move(node));
    }

    /// Updates the name of one of the child nodes.
    /// This function DOES NOT UPDATE THE NAME OF THE NODE itself, just the name by which the parent knows it.
    /// @param node         Node to rename, must be part of the container and still have its old name.
    /// @param new_name     New name of the node.
    static void rename_child(NodeContainer& container, valid_ptr<const SceneNode*> node, std::string name)
    {
        container._rename(node, std::move(name));
    }
};

NOTF_CLOSE_NAMESPACE
