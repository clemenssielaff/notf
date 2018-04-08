#pragma once

#include "app/forwards.hpp"
#include "common/hash.hpp"
#include "common/map.hpp"
#include "common/mutex.hpp"
#include "common/pointer.hpp"
#include "common/size2.hpp"

NOTF_OPEN_NAMESPACE

// ===================================================================================================================//

/// The Scene is an abstract hierarchy of scene nodes.
/// We keep it abstract, so that a Scene can both be a map of nested 2D rectangles, as well as a 3D scene graph or
/// whatever else.
///
/// Events
/// ======
///
/// The Scene knows how to handle (untyped) events via the `handle_event` method. All events are ignored by default.
/// When you override the handler method, use event->type() to get a type integer that you can use in a
/// switch-statement.
///
/// Scene Hierarchy
/// ===============
///
/// Scene nodes form a hierarchy with a single RootNode on top. Each node can have 0-n children, although different
/// subtypes may pose additional restrictions.
///
/// Only a concrete nodes may create other nodes. The only method allowed to do so is `Scene::Node::_add_child` which
/// returns a `NodeHandle<T>` (with T being the type of the child node). Handles own the actual child node, meaning that
/// when they are destroyed, so is their child SceneNode. You store them in the parenting node, in any way that
/// makes most sense for the type of node (list, map, quadtree...). They are not copy- or assignable, think of them as
/// specialized unique_ptr.
///
/// To traverse the hierarchy, we need to know all of the children of a node and their draw order, from back to front
/// (earlier nodes are drawn behind later nodes). Therefore, every scene node contains a vector of raw pointers to their
/// children. By default, `_add_child` appends the newly created SceneNode to the vector, resulting in each new child
/// being drawn in front of its next-older sibling. You are free to reshuffle the children as you please using the
/// dedicated functions on `Scene::Node`. The `m_children` vector is private so you can't mess it up ;)
///
/// Nodes also have a link back to their parent, but do not own the parent (obviously). As parents are guaranteed to
/// outlive their children in the tree, we can be certain that all nodes have a valid parent. The RootNode is its
/// own parent, so while it is in fact valid you need to check for this condition when traversing up the hierarchy,
/// otherwise you'll be stuck in an infinite loop (then again, if you don't check for the stop condition and the parent
/// is null, your program crashes).
///
/// Name
/// ====
///
/// Each Node has a name. Be default, the name is of the format "SceneNode#[number]" where the number is guaranteed to
/// be unique. The user can change the name of each node, in which case it might no longer be unique.
///
/// Signals
/// =======
///
/// Scene nodes communicate with each other either through their relationship in the hierachy (parents to children and
/// vice-versa) or via Signals. Signals have the advantage of being able to connect every node in the same SceneManager
/// regardless of its position in the hierarchy. They can be created by the user and enabled/disabled at will. In order
/// to facilitate Signal handling at the lowest possible level, all Nodes derive from the `receive_signals` class
/// that takes care of removing leftover connections that still exist once the Node goes out of scope.
///
class Scene {

    // types ---------------------------------------------------------------------------------------------------------//
public:
    struct Node; // forward

private:
    struct RootNode; // forward

    using ChildContainer = std::vector<valid_ptr<Node*>>;
    using ChildContainerPtr = std::unique_ptr<ChildContainer>;

    //=========================================================================
public:
    /// A NodeHandle is the user-facing part of a scene node instance.
    /// It is what a call to `BaseNode::_add_child` returns and manages the lifetime of the created child.
    /// NodeHandle are not copyable or assignable to make sure that the parent node always outlives its child nodes.
    /// The handle is templated on the BaseNode subtype that it contains, so the owner has direct access to the node
    /// and doesn't manually need to cast.
    template<typename T, typename = std::enable_if_t<std::is_base_of<Node, T>::value>>
    struct NodeHandle {

        friend struct Node;

        // methods ------------------------------------------------------------
    private:
        /// Constructor.
        /// @param scene    Scene to manage the node.
        /// @param node     Managed BaseNode instance.
        NodeHandle(valid_ptr<Scene*> scene, valid_ptr<Node*> node) : m_scene(scene), m_node(node) {}

    public:
        NOTF_NO_COPY_OR_ASSIGN(NodeHandle)

        /// Default constructor.
        NodeHandle() = default;

        /// Move constructor.
        /// @param other    NodeHandle to move from.
        NodeHandle(NodeHandle&& other) : m_scene(other.m_scene), m_node(other.m_node)
        {
            other.m_scene = nullptr;
            other.m_node = nullptr;
        }

        /// Move assignment operator.
        /// @param other    NodeHandle to move from.
        NodeHandle& operator=(NodeHandle&& other)
        {
            m_scene = other.m_scene;
            m_node = other.m_node;
            other.m_scene = nullptr;
            other.m_node = nullptr;
        }

        /// Destructor.
        ~NodeHandle() NOTF_THROWS_IF(is_debug_build())
        {
            if (!m_scene) {
                return;
            }
            else {
                std::lock_guard<RecursiveMutex> lock(m_scene->m_mutex);
                NOTF_ASSERT_MSG(m_scene->m_nodes.count(m_node) != 0, "Node double deletion detected");

                valid_ptr<Node*> parent = m_node->parent();
                NOTF_ASSERT_MSG(m_scene->m_nodes.count(parent) != 0, "A node must not outlive its parent");

                // remove your node immediately, if the scene is not frozen
                if (!m_scene->is_frozen()) {
                    auto it = m_scene->m_nodes.find(m_node);
                    NOTF_ASSERT_MSG(it != m_scene->m_nodes.end(), "Cannot remove unknown node from scene");
                    it.value().reset(); // delete the node before modifying the `m_nodes` map
                    m_scene->m_nodes.erase(it);
                }

                { // remove yourself from your parent node
                    ChildContainer& siblings = parent->write_children();
                    auto it = std::find(siblings.begin(), siblings.end(), m_node);
                    NOTF_ASSERT_MSG(it != siblings.end(), "Cannot remove unknown child node from parent");
                    siblings.erase(it);
                }
            }
        }

        /// @{
        /// The managed BaseNode instance correctly typed.
        T* get() { return static_cast<T*>(m_node.get()); }
        const T* get() const { return static_cast<const T*>(m_node.get()); }
        /// @}

        // fields -------------------------------------------------------------
    private:
        /// The scene containing this node.
        risky_ptr<Scene*> m_scene = nullptr;

        /// Managed BaseNode instance.
        risky_ptr<Node*> m_node = nullptr;
    };

    //=========================================================================

    /// Base of all Nodes in a Scene.
    /// Contains a handle to its children and the Scene from which to request them.
    /// This allows us to freeze the Scene hierarchy during rendering with the only downside of an additional
    /// indirection between the node and its children.
    struct Node {

        friend struct RootNode;

        // methods ------------------------------------------------------------
    public:
        NOTF_NO_COPY_OR_ASSIGN(Node)

        /// Constructor.
        /// @param scene    Scene to manage the node.
        /// @param parent   Parent of this node.
        Node(Scene& scene, valid_ptr<Node*> parent);

        /// Destructor.
        virtual ~Node() NOTF_THROWS_IF(is_debug_build());

        /// @{
        /// The Scene containing this node.
        Scene& scene() { return m_scene; }
        const Scene& scene() const { return m_scene; }
        /// @}
        ///
        /// @{
        /// The parent of this node.
        valid_ptr<Node*> parent() { return m_parent; }
        valid_ptr<const Node*> parent() const { return m_parent; }
        /// @}

        /// All children of this node, orded from back to front.
        /// Never creates a delta.
        const ChildContainer& read_children() const;

        /// All children of this node, orded from back to front.
        /// Will create a new delta if the scene is frozen.
        ChildContainer& write_children();

        /// The user-defined name of this node, is potentially empty and not guaranteed to be unique if set.
        const std::string& name() const { return m_name; }

        /// Tests, if this Node is a descendant of the given ancestor.
        bool has_ancestor(const valid_ptr<const Node*> ancestor) const;

        ///@{
        /// Finds and returns the first common ancestor of two Nodes.
        /// The root node is always a common ancestor, iff the two nodes are in the same scene.
        /// @throws hierarchy_error If the two nodes are not in the same scene.
        valid_ptr<Node*> common_ancestor(valid_ptr<Node*> other);
        valid_ptr<const Node*> common_ancestor(valid_ptr<const Node*> other) const
        {
            return const_cast<Node*>(this)->common_ancestor(const_cast<Node*>(other.get()));
        }
        ///@}

        ///@{
        /// Returns the first ancestor of this Node that has a specific type (can be empty if none is found).
        template<typename T>
        risky_ptr<const T*> first_ancestor() const
        {
            if (!std::is_base_of<Node, T>::value) {
                return nullptr;
            }
            valid_ptr<const Node*> next = parent();
            while (next != next->parent()) {
                if (const T* result = dynamic_cast<T*>(next)) {
                    return result;
                }
                next = next->parent();
            }
            return nullptr;
        }
        template<typename T>
        risky_ptr<T*> first_ancestor()
        {
            return const_cast<const Node*>(this)->first_ancestor<T>();
        }
        ///@}

        /// Updates the name of this Node.
        /// @param name New name of this Node.
        const std::string& set_name(std::string name);

        /// Moves this Node in front of all of its siblings.
        void stack_front();

        /// Moves this Node behind all of its siblings.
        void stack_back();

        /// Moves this Node before a given sibling.
        /// @param sibling  Sibling to stack before.
        /// @throws hierarchy_error If the sibling is not a sibling of this node.
        void stack_before(valid_ptr<Node*> sibling);

        /// Moves this Node behind a given sibling.
        /// @param sibling  Sibling to stack behind.
        /// @throws hierarchy_error If the sibling is not a sibling of this node.
        void stack_behind(valid_ptr<Node*> sibling);

    protected:
        /// Creates and adds a new child to this node.
        /// @param args Arguments that are forwarded to the constructor of the child.
        template<typename T, typename... Args>
        NodeHandle<T> _add_child(Args&&... args)
        {
            std::unique_ptr<Node> new_child = std::make_unique<T>(std::forward<Args>(args)...);
            Node* child_handle = new_child.get();
            {
                std::lock_guard<RecursiveMutex> lock(m_scene.m_mutex);
                m_scene.m_nodes.emplace(std::make_pair(child_handle, std::move(new_child)));
                write_children().emplace_back(child_handle);
            }
            return NodeHandle<T>(&m_scene, child_handle);
        }

    private:
        /// Prepares an empty vector for the children of a new node.
        /// @param scene    Scene to manage the children.
        static valid_ptr<ChildContainer*> _prepare_children(Scene& scene);

        // fields -------------------------------------------------------------
    private:
        /// The scene containing this node.
        Scene& m_scene;

        /// The parent of this node.
        /// Is guaranteed to outlive this node.
        valid_ptr<Node*> const m_parent;

        /// Handle of a vector of all children of this node, ordered from back to front.
        valid_ptr<ChildContainer*> const m_children;

        /// The user-defined name of this Node, is potentially empty and not guaranteed to be unique if set.
        std::string m_name;
    };

    //=========================================================================
private:
    /// The singular root node of a Scene hierarchy.
    struct RootNode : public Node {

        /// Constructor.
        /// @param scene    Scene to manage the node.
        RootNode(Scene& scene);

        /// Destructor.
        virtual ~RootNode() NOTF_THROWS_IF(is_debug_build());

        /// Creates and adds a new child to this node.
        /// @param args Arguments that are forwarded to the constructor of the child.
        template<typename T, typename... Args>
        NodeHandle<T> add_child(Args&&... args)
        {
            return _add_child<T>(std::forward<Args>(args)...);
        }
    };

    //=========================================================================

    /// RAII object to make sure that a frozen scene is ALWAYS unfrozen again
    struct NOTF_NODISCARD FreezeGuard {

        NOTF_NO_COPY_OR_ASSIGN(FreezeGuard)
        NOTF_NO_HEAP_ALLOCATION(FreezeGuard)

        /// Constructor.
        /// @param scene        Scene to freeze.
        /// @param thread_id    ID of the freezing thread, uses the calling thread by default. (exposed for testability)
        FreezeGuard(Scene& scene, const std::thread::id thread_id = std::this_thread::get_id());

        /// Destructor.
        ~FreezeGuard() NOTF_THROWS_IF(is_debug_build());

        // fields -------------------------------------------------------------
    private:
        /// Scene to freeze.
        Scene* m_scene;
    };

    //=========================================================================
public:
    /// Thrown when a node did not have the expected position in the hierarchy.
    NOTF_EXCEPTION_TYPE(hierarchy_error)

    // methods -------------------------------------------------------------------------------------------------------//
protected:
    /// Constructor.
    /// @param manager  The SceneManager owning this Scene.
    Scene(SceneManager& manager);

public:
    NOTF_NO_COPY_OR_ASSIGN(Scene)

    /// Destructor.
    virtual ~Scene() NOTF_THROWS_IF(is_debug_build());

    /// @{
    /// The SceneManager owning this Scene.
    SceneManager& manager() { return m_manager; }
    const SceneManager& manager() const { return m_manager; }
    /// @}

    /// @{
    /// The unique root node of this Scene.
    RootNode& root() { return *m_root; }
    const RootNode& root() const { return *m_root; }
    /// @}

    // event handling ---------------------------------------------------------

    /// Handles an untyped event.
    /// @returns True iff the event was handled.
    virtual bool handle_event(Event& event NOTF_UNUSED) { return false; }

    /// Resizes the view on this Scene.
    /// @param size Size of the view in pixels.
    virtual void resize_view(Size2i size) = 0;

    // scene hierarchy --------------------------------------------------------
private:
    /// Checks if the scene is currently frozen or not.
    bool is_frozen() const NOTF_THROWS_IF(is_debug_build())
    {
        NOTF_ASSERT(m_mutex.is_locked_by_this_thread());
        return m_render_thread != std::thread::id();
    }

    /// Checks if the calling thread is the current render thread.
    bool is_render_thread(const std::thread::id thread_id) const NOTF_THROWS_IF(is_debug_build())
    {
        NOTF_ASSERT(m_mutex.is_locked_by_this_thread());
        return thread_id == m_render_thread;
    }

private:
    /// Creates the root node.
    RootNode* _create_root();

    // fields --------------------------------------------------------------------------------------------------------//
private:
    /// The SceneManager owning this Scene.
    SceneManager& m_manager;

    /// Mutex guarding the scene.
    /// Has to be recursive, because when we delete a node, it will delete its child nodes and we need to protect the
    /// destructor with a lock.
    mutable RecursiveMutex m_mutex;

    /// Map owning all nodes in the scene.
    robin_map<valid_ptr<const Node*>, std::unique_ptr<Node>, PointerHash<Node>> m_nodes;

    /// Mapping from all nodes in the scene to their children.
    /// Is separate from the Node type so the Scene hierarchy can be frozen.
    robin_map<valid_ptr<const ChildContainer*>, ChildContainerPtr, PointerHash<ChildContainer>> m_child_container;

    /// Map containing copieds of ChildContainer that were modified while the Scene was frozen.
    robin_map<valid_ptr<const ChildContainer*>, ChildContainerPtr, PointerHash<ChildContainer>> m_deltas;

    /// Thread id of the render thread, if we are rendering at the moment
    /// (a value != default signifies that the graph is currently frozen).
    std::thread::id m_render_thread;

    /// The singular root node of the scene hierarchy.
    valid_ptr<RootNode*> m_root;
};

NOTF_CLOSE_NAMESPACE
