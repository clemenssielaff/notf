#pragma once

#include "app/forwards.hpp"
#include "common/hash.hpp"
#include "common/map.hpp"
#include "common/mutex.hpp"
#include "common/pointer.hpp"
#include "common/size2.hpp"

NOTF_OPEN_NAMESPACE

// ===================================================================================================================//

class Scene {

    // types ---------------------------------------------------------------------------------------------------------//
private:
    struct Node; // forward
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

        // methods ------------------------------------------------------------
    private:
        /// Constructor.
        /// @param scene    Scene to manage the node.
        /// @param node     Managed BaseNode instance.
        NodeHandle(Scene& scene, valid_ptr<Node*> node) : m_scene(scene), m_node(node) {}

    public:
        NOTF_NO_COPY_OR_ASSIGN(NodeHandle)

#ifdef NOTF_DEBUG
        /// Destructor.
        ~NodeHandle() NOTF_THROWS
        {
            std::lock_guard<Mutex> lock(m_scene.m_mutex);
            NOTF_ASSERT_MSG(m_scene.m_nodes.count(m_node) != 0, "Node double deletion detected");
            NOTF_ASSERT_MSG(!m_node->read_children().empty(), "A node must not be deleted before its children");

            valid_ptr<Node*> parent = m_node->parent();
            NOTF_ASSERT_MSG(m_scene.m_nodes.count(parent) != 0, "A node must not outlive its parent");

            { // remove yourself from your parent node
                ChildContainer& siblings = parent->write_children();
                auto it = std::find(siblings.begin(), siblings.end(), m_node);
                NOTF_ASSERT_MSG(it != siblings.end(), "Cannot remove unknown child node from parent");
                siblings.erase(it);
            }

            // remove your node immediately, if the scene is not frozen
            if (!m_scene.is_frozen()) {
                auto it = m_scene.m_nodes.find(m_node);
                NOTF_ASSERT_MSG(it != m_scene.m_nodes.end(), "Cannot remove unknown node from scene");
                m_scene.m_nodes.erase(it);
            }
        }
#else
        /// Destructor.
        ~Node()
        {
            valid_ptr<Node*> parent = m_node->parent();
            {
                std::lock_guard<Mutex> lock(m_scene.m_mutex);

                { // remove yourself from your parent node
                    ChildContainer& siblings = parent->write_children();
                    siblings.erase(std::find(siblings.begin(), siblings.end(), m_node));
                }

                // remove your node immediately, if the scene is not frozen
                if (!m_scene.is_frozen()) {
                    m_scene.m_nodes.erase(m_scene.m_nodes.find(m_node));
                }
            }
        }
#endif

        /// @{
        /// The managed BaseNode instance correctly typed.
        T* get() { return static_cast<T*>(m_node.get()); }
        const T* get() const { return static_cast<const T*>(m_node.get()); }
        /// @}

        // fields -------------------------------------------------------------
    private:
        /// The scene containing this node.
        Scene& m_scene;

        /// Managed BaseNode instance.
        valid_ptr<Node*> const m_node;
    };

    //=========================================================================
private:
    /// Base of all Nodes in a Scene.
    /// Contains a handle to its children and the Scene from which to request them.
    /// This allows us to freeze the Scene hierarchy during rendering with the only downside of an additional
    /// indirection between the node and its children.
    struct Node {

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
                std::lock_guard<Mutex> lock(m_scene.m_mutex);
                m_scene.m_nodes.emplace(std::make_pair(child_handle, std::move(new_child)));
                write_children().emplace_back(child_handle);
            }
            return NodeHandle<T>(m_scene, child_handle);
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
    Scene() = default;

public:
    NOTF_NO_COPY_OR_ASSIGN(Scene)

    /// Destructor.
    virtual ~Scene();

    // event handling ---------------------------------------------------------

    /// Handles an untyped event.
    /// @returns True iff the event was handled.
    virtual bool handle_event(Event& event) = 0;

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

    // fields --------------------------------------------------------------------------------------------------------//
private:
    /// Map owning all nodes in the scene.
    robin_map<valid_ptr<const Node*>, std::unique_ptr<Node>, PointerHash<Node>> m_nodes;

    /// Mapping from all nodes in the scene to their children.
    /// Is separate from the Node type so the Scene hierarchy can be frozen.
    robin_map<valid_ptr<const ChildContainer*>, ChildContainerPtr, PointerHash<ChildContainer>> m_child_container;

    /// Map containing copieds of ChildContainer that were modified while the Scene was frozen.
    robin_map<valid_ptr<const ChildContainer*>, ChildContainerPtr, PointerHash<ChildContainer>> m_deltas;

    /// Mutex guarding the scene.
    mutable Mutex m_mutex;

    /// Thread id of the render thread, if we are rendering at the moment
    /// (a value != default signifies that the graph is currently frozen).
    std::thread::id m_render_thread;
};

NOTF_CLOSE_NAMESPACE
