#pragma once

#include <algorithm>
#include <unordered_map>
#include <unordered_set>

#include "app/forwards.hpp"
#include "common/assert.hpp"
#include "common/hash.hpp"
#include "common/mutex.hpp"
#include "common/pointer.hpp"
#include "common/signal.hpp"
#include "common/size2.hpp"

NOTF_OPEN_NAMESPACE

// ===================================================================================================================//

/// Base of all Nodes in a Scene.
/// Contains a handle to its children and the Scene from which to request them.
/// This allows us to freeze the Scene hierarchy during rendering with the only downside of an additional
/// indirection between the node and its children.
class SceneNode : public receive_signals {

    // types --------------------------------------------------------------
public:
    NOTF_ACCESS_TYPES(Scene)

    /// Container used to store the children of a SceneNode.
    using ChildContainer = std::vector<valid_ptr<SceneNode*>>;

protected:
    /// Token object to make sure that Node instances can only be created by a call to `_add_child`.
    class Token {
        friend class SceneNode;
        Token() {} // not `= default`, otherwise you could construct a Token via `Token{}`.
    };

    //=========================================================================
public:
    /// Thrown when a node did not have the expected position in the hierarchy.
    NOTF_EXCEPTION_TYPE(hierarchy_error)

    // signals ------------------------------------------------------------
public:
    /// Emitted when this Node changes its name.
    /// @param The new name.
    Signal<const std::string&> on_name_changed;

    // methods ------------------------------------------------------------
public:
    NOTF_NO_COPY_OR_ASSIGN(SceneNode)

    /// Constructor.
    /// @param token    Factory token provided by Node::_add_child.
    /// @param scene    Scene to manage the node.
    /// @param parent   Parent of this node.
    SceneNode(const Token&, Scene& scene, valid_ptr<SceneNode*> parent);

    /// Destructor.
    virtual ~SceneNode();

    /// @{
    /// The Scene containing this node.
    Scene& scene() { return m_scene; }
    const Scene& scene() const { return m_scene; }
    /// @}
    ///
    /// @{
    /// The parent of this node.
    valid_ptr<SceneNode*> parent() { return m_parent; }
    valid_ptr<const SceneNode*> parent() const { return m_parent; }
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
    bool has_ancestor(const valid_ptr<const SceneNode*> ancestor) const;

    ///@{
    /// Finds and returns the first common ancestor of two Nodes.
    /// The root node is always a common ancestor, iff the two nodes are in the same scene.
    /// @throws hierarchy_error If the two nodes are not in the same scene.
    valid_ptr<SceneNode*> common_ancestor(valid_ptr<SceneNode*> other);
    valid_ptr<const SceneNode*> common_ancestor(valid_ptr<const SceneNode*> other) const
    {
        return const_cast<SceneNode*>(this)->common_ancestor(const_cast<SceneNode*>(other.get()));
    }
    ///@}

    ///@{
    /// Returns the first ancestor of this Node that has a specific type (can be empty if none is found).
    template<typename T>
    risky_ptr<const T*> first_ancestor() const
    {
        if (!std::is_base_of<SceneNode, T>::value) {
            return nullptr;
        }
        valid_ptr<const SceneNode*> next = parent();
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
        return const_cast<const SceneNode*>(this)->first_ancestor<T>();
    }
    ///@}

    /// Updates the name of this Node.
    /// @param name New name of this Node.
    const std::string& set_name(std::string name);

    // z-order ------------------------------------------------------------

    /// Checks if this Node is in front of all of its siblings.
    bool is_in_front() const;

    /// Checks if this Node is behind all of its siblings.
    bool is_in_back() const;

    /// Returns true if this node is stacked anywhere in front of the given sibling.
    /// @param sibling  Sibling node to test against.
    /// @throws hierarchy_error If the sibling is not a sibling of this node.
    bool is_in_front_of(const valid_ptr<SceneNode*> sibling) const;

    /// Returns true if this node is stacked anywhere behind the given sibling.
    /// @param sibling  Sibling node to test against.
    /// @throws hierarchy_error If the sibling is not a sibling of this node.
    bool is_behind(const valid_ptr<SceneNode*> sibling) const;

    /// Moves this Node in front of all of its siblings.
    void stack_front();

    /// Moves this Node behind all of its siblings.
    void stack_back();

    /// Moves this Node before a given sibling.
    /// @param sibling  Sibling to stack before.
    /// @throws hierarchy_error If the sibling is not a sibling of this node.
    void stack_before(const valid_ptr<SceneNode*> sibling);

    /// Moves this Node behind a given sibling.
    /// @param sibling  Sibling to stack behind.
    /// @throws hierarchy_error If the sibling is not a sibling of this node.
    void stack_behind(const valid_ptr<SceneNode*> sibling);

protected:
    /// Creates and adds a new child to this node.
    /// @param args Arguments that are forwarded to the constructor of the child.
    ///             Note that all arguments for the Node base class are supplied automatically by this method.
    template<typename T, typename... Args>
    NodeHandle<T> _add_child(Args&&... args);

private:
    /// Registers a new node with the scene.
    /// @param scene    Scene to manage the children.
    /// @returns        An new empty vector for the children of this node.
    valid_ptr<ChildContainer*> _register_with_scene(Scene& scene);

    // fields -------------------------------------------------------------
private:
    /// The scene containing this node.
    Scene& m_scene;

    /// The parent of this node.
    /// Is guaranteed to outlive this node.
    valid_ptr<SceneNode*> const m_parent;

    /// Handle of a vector of all children of this node, ordered from back to front.
    valid_ptr<ChildContainer*> const m_children;

    /// The user-defined name of this Node, is potentially empty and not guaranteed to be unique if set.
    std::string m_name;
};

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
/// when they are destroyed, so is their child Scene Node. You store them in the parenting node, in any way that
/// makes most sense for the type of node (list, map, quadtree...). They are not copy- or assignable, think of them as
/// specialized unique_ptr.
///
/// To traverse the hierarchy, we need to know all of the children of a node and their draw order, from back to front
/// (earlier nodes are drawn behind later nodes). Therefore, every scene node contains a vector of raw pointers to their
/// children. By default, `_add_child` appends the newly created Scene Node to the vector, resulting in each new child
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

    template<typename>
    friend struct NodeHandle;

    friend class SceneNode;

    // types ---------------------------------------------------------------------------------------------------------//
public:
    NOTF_ACCESS_TYPES(test::Harness)

private:
    using ChildContainer = SceneNode::ChildContainer;

    //=========================================================================

    /// The singular root node of a Scene hierarchy.
    struct RootNode : public SceneNode {

        /// Constructor.
        /// @param token    Factory token provided by the Scene.
        /// @param scene    Scene to manage the node.
        RootNode(const Token& token, Scene& scene);

        /// Destructor.
        virtual ~RootNode();

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

        /// Move constructor.
        /// @param other    FreezeGuard to move from.
        FreezeGuard(FreezeGuard&& other) : m_scene(other.m_scene) { other.m_scene = nullptr; }

        /// Destructor.
        ~FreezeGuard();

        // fields -------------------------------------------------------------
    private:
        /// Scene to freeze.
        Scene* m_scene;

        /// Id of the freezing thread.
        const std::thread::id m_thread_id;
    };

    // methods -------------------------------------------------------------------------------------------------------//
protected:
    /// Constructor.
    /// @param manager  The SceneManager owning this Scene.
    Scene(SceneManager& manager);

public:
    NOTF_NO_COPY_OR_ASSIGN(Scene)

    /// Destructor.
    virtual ~Scene();

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
    /// Freezes the Scene if it is not already frozen.
    /// @param thread_id    Id of the calling thread.
    /// @returns            True iff the Scene was frozen.
    bool freeze(const std::thread::id thread_id);

    /// Unfreezes the Scene again.
    void unfreeze(const std::thread::id thread_id);

    /// Checks if the scene is currently frozen or not.
    bool _is_frozen() const
    {
        NOTF_ASSERT(m_mutex.is_locked_by_this_thread());
        return m_render_thread != std::thread::id();
    }

    /// Checks if the calling thread is the current render thread.
    bool _is_render_thread(const std::thread::id thread_id) const
    {
        NOTF_ASSERT(m_mutex.is_locked_by_this_thread());
        return thread_id == m_render_thread;
    }

    /// Deletes the given node from the Scene.
    /// @param node     Node to remove.
    void _delete_node(valid_ptr<SceneNode*> node);

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
    std::unordered_map<valid_ptr<const SceneNode*>, std::unique_ptr<SceneNode>, PointerHash<SceneNode>> m_nodes;

    /// Mapping from all nodes in the scene to their children.
    /// Is separate from the Node type so the Scene hierarchy can be frozen.
    std::unordered_map<valid_ptr<const ChildContainer*>, std::unique_ptr<ChildContainer>, PointerHash<ChildContainer>>
        m_child_container;

    /// Map containing copieds of ChildContainer that were modified while the Scene was frozen.
    std::unordered_map<valid_ptr<const ChildContainer*>, std::unique_ptr<ChildContainer>, PointerHash<ChildContainer>>
        m_deltas;

    /// All nodes that were deleted while the Scene was frozen.
    std::unordered_set<valid_ptr<SceneNode*>> m_deletion_deltas;

    /// Thread id of the render thread, if we are rendering at the moment
    /// (a value != default signifies that the graph is currently frozen).
    std::thread::id m_render_thread;

    /// The singular root node of the scene hierarchy.
    valid_ptr<RootNode*> m_root;
};

// ===================================================================================================================//

/// A NodeHandle is the user-facing part of a scene node instance.
/// It is what a call to `BaseNode::_add_child` returns and manages the lifetime of the created child.
/// NodeHandle are not copyable or assignable to make sure that the parent node always outlives its child nodes.
/// The handle is templated on the BaseNode subtype that it contains, so the owner has direct access to the node
/// and doesn't manually need to cast.
template<typename T>
struct NodeHandle {
    static_assert(std::is_base_of<SceneNode, T>::value, "The type wrapped by NodeHandle<T> must be a subclass of Node");

    template<typename U, typename... Args>
    friend NodeHandle<U> SceneNode::_add_child(Args&&... args);

    // methods ------------------------------------------------------------
private:
    /// Constructor.
    /// @param scene    Scene to manage the node.
    /// @param node     Managed BaseNode instance.
    NodeHandle(valid_ptr<Scene*> scene, valid_ptr<SceneNode*> node) : m_scene(scene), m_node(node) {}

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
        return *this;
    }

    /// Destructor.
    ~NodeHandle()
    {
        if (!m_scene) {
            return;
        }
        std::lock_guard<RecursiveMutex> lock(m_scene->m_mutex);

        valid_ptr<SceneNode*> parent = m_node->parent();
        NOTF_ASSERT_MSG(m_scene->m_nodes.count(parent) != 0,
                        "The parent of Node \"{}\" was deleted before its child was! \n"
                        "Have you been storing NodeHandles of child nodes outside their parent?",
                        m_node->name());

        // remove yourself as a child from your parent node
        {
            SceneNode::ChildContainer& siblings = parent->write_children(); // this creates a delta in a frozen scene
            auto child_it = std::find(siblings.begin(), siblings.end(), m_node);
            NOTF_ASSERT_MSG(child_it != siblings.end(), "Cannot remove unknown child node from parent");
            siblings.erase(child_it);
        }

        // if this scene is currently frozen, add a deletion delta
        if (m_scene->_is_frozen()) {
            m_scene->m_deletion_deltas.insert(m_node);
        }

        // if the scene is not frozen, we can remove the node immediately
        else {
            m_scene->_delete_node(m_node);
        }
    }

    /// @{
    /// The managed BaseNode instance correctly typed.
    constexpr T* operator->() { return static_cast<T*>(m_node.get()); }
    constexpr const T* operator->() const { return static_cast<const T*>(m_node.get()); }
    /// @}

    /// @{
    /// The managed BaseNode instance correctly typed.
    T& operator*() { return *static_cast<T*>(m_node.get()); }
    const T& operator*() const { return *static_cast<T*>(m_node.get()); }
    /// @}

    // fields -------------------------------------------------------------
private:
    /// The scene containing this node.
    risky_ptr<Scene*> m_scene = nullptr;

    /// Managed BaseNode instance.
    risky_ptr<SceneNode*> m_node = nullptr;
};

// ===================================================================================================================//

template<typename T, typename... Args>
NodeHandle<T> SceneNode::_add_child(Args&&... args)
{
    static const Token token;
    std::unique_ptr<SceneNode> new_child = std::make_unique<T>(token, m_scene, this, std::forward<Args>(args)...);
    SceneNode* child_handle = new_child.get();
    {
        std::lock_guard<RecursiveMutex> lock(m_scene.m_mutex);
        m_scene.m_nodes.emplace(std::make_pair(child_handle, std::move(new_child)));
        write_children().emplace_back(child_handle);
    }
    return NodeHandle<T>(&m_scene, child_handle);
}

// ===================================================================================================================//

template<>
class SceneNode::Access<Scene> {
    friend class Scene;

    /// Creates a factory Token so the Scene can create its RootNode.
    static SceneNode::Token create_token() { return Token{}; }
};

// ===================================================================================================================//

#ifdef NOTF_TEST

template<>
class Scene::Access<test::Harness> {
public:
    /// Constructor.
    /// @param scene    Scene to access.
    Access(Scene& scene) : m_scene(scene) {}

    /// Creates and returns a FreezeGuard that keeps the scene frozen while it is alive.
    /// @param thread_id    Id of the freezing thread.
    Scene::FreezeGuard freeze_guard(const std::thread::id thread_id = std::this_thread::get_id())
    {
        return Scene::FreezeGuard(m_scene, thread_id);
    }

    /// Returns the number of nodes in the Scene.
    size_t node_count() { return m_scene.m_nodes.size(); }

    /// Returns the number of child container in the Scene.
    size_t child_container_count() { return m_scene.m_child_container.size(); }

    /// Returns the number of deltas in the Scene.
    size_t delta_count() { return m_scene.m_deltas.size(); }

    /// Returns the number of new nodes in the Scene.
    size_t deletions_count() { return m_scene.m_deletion_deltas.size(); }

    /// Freezes the Scene.
    void freeze(const std::thread::id thread_id = std::this_thread::get_id()) { m_scene.freeze(thread_id); }

    /// Unfreezes the Scene.
    void unfreeze(const std::thread::id thread_id = std::this_thread::get_id()) { m_scene.unfreeze(thread_id); }

    /// Lets the caller pretend that this is the render thread.
    void set_render_thread(const std::thread::id thread_id) { m_scene.m_render_thread = thread_id; }

    // fields -----------------------------------------------------------------
private:
    /// Scene to access.
    Scene& m_scene;
};

#else
template<>
class Scene::Access<test::Harness> {};
#endif // #ifdef NOTF_TEST

NOTF_CLOSE_NAMESPACE
