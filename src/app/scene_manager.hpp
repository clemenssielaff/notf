#pragma once

#include "app/forwards.hpp"
#include "common/hash.hpp"
#include "common/map.hpp"
#include "common/mutex.hpp"
#include "common/pointer.hpp"

NOTF_OPEN_NAMESPACE

// ===================================================================================================================//

/// CRTP mixin class for all SceneNodes that makes sure that each concrete SceneNode subclass implements the minimum set
/// of methods required.
template<typename T, typename = std::enable_if_t<std::is_base_of<detail::SceneNodeBase, T>::value>>
struct SceneNodeInterface {

    /// Implements the pure virtual `clone` method via the copy constructor.
    virtual std::unique_ptr<detail::SceneNodeBase> clone() const
    {
        return std::make_unique<T>(static_cast<const T&>(*this));
    }
};

//====================================================================================================================//

namespace detail {

NOTF_DISABLE_WARNING("unused-template")

/// Utility function to make sure that a SceneNode subtype can be used by the SceneManager.
template<typename T>
constexpr static bool is_valid_scene_node_type()
{
    constexpr bool is_scene_node = std::is_base_of<SceneNode, T>::value;
    constexpr bool is_copy_constructible = std::is_copy_constructible<T>::value;
    constexpr bool is_move_constructible = std::is_move_constructible<T>::value;

    static_assert(is_scene_node, "Each SceneNode subtype must derive from `SceneNode`");
    static_assert(is_copy_constructible, "A `SceneNode` subtype must be copy constructible");
    static_assert(is_move_constructible, "A `SceneNode` subtype must be move constructible");

    return is_scene_node && is_copy_constructible && is_move_constructible;
}

NOTF_ENABLE_WARNINGS

// ===================================================================================================================//

/// SceneNode baseclass, used for management types like SceneManager::DeletionDelta.
class SceneNodeBase {

    // methods -------------------------------------------------------------------------------------------------------//
public:
    /// Value Constructor.
    /// @param manager      SceneManager owning this SceneNode.
    SceneNodeBase(SceneManager& manager) : m_manager(manager) {}

    /// Destructor.
    virtual ~SceneNodeBase();

    /// Every SceneNode must be able to clone itself.
    virtual std::unique_ptr<SceneNodeBase> clone() const = 0;

    ///@{
    /// The SceneManager owning this node.
    SceneManager& manager() { return m_manager; }
    const SceneManager& manager() const { return m_manager; }
    ///@}

    // fields --------------------------------------------------------------------------------------------------------//
protected:
    /// SceneManager owning this SceneNode.
    SceneManager& m_manager;
};

} // namespace detail

// ===================================================================================================================//

///
///
/// State
/// =====
///
/// The SceneManager has State objects that defines how to render a frame.
/// A State is made up of a list of Layers.
/// Layers define an AABR (potentially full-screen) that are rendered into the screen buffer on each frame.
/// Each Layer has a single Renderer (short: Producer) that define their content.
/// Producers can either generate their content procedurally or display a RenderTarget.
/// RenderTargets have a Producer each, while Producers can themselves refer to 0-n other RenderTargets.
/// RenderTarget may not depend on a Producer which itself depends on the same RenderTarget (no loops).
///
///    ------ Layers are rendered from left to right ------>
///
///        Layer1     Layer2                 Layer3           |
///        ------     ------                 ------           |
///          |          |                      |              |
///      Renderer1  Renderer2              Renderer3          |
///                    +----------+     +------+----+       depends
///                            RenderTarget1        |         on
///                                 |               |         |
///                             Renderer4           |         |
///                                 +--------+      |         |
///                                        RenderTarget2      V
///                                              |
///                                          Renderer5
///
/// Threading
/// =========
///
/// One important design decision concerned the threading model with regards to rendering.
/// Obviously we need the actual rendering (OpenGL calls) made from a dedicated thread, in case OpenGL blocks to draw
/// a more complicated frame. During that time, even though the UI cannot update visually, we need the rest of the
/// application to remain responsive.
///
/// Ideally, that is all that the render thread does - take some sort of fixed state, compile the best arrangement of
/// OpenGL calls to satisfy the requirements imposed by the state and execute those calls.
/// Practically however, this is a bit more complicated.
///
/// Some Renderer may require only properties in order to draw: the "smoke" FragmentProducer for example,
/// requires only the screen resolution and the time to update.
/// In that case, it is enough for the Application to update the PropertyGraph with all of its accumulated updates
/// from various threads and then kick off the SceneManager of each Window.
///
///                     +
///                     |     (owned by Application)         (owned by Window)
///                     |              |                            |
///           +---+     |              v                            v
///               |     |     +------------------+          +----------------+
///     various   |   async   |                  |   sync   |                |
///               +----------->  PropertyGraph   +---------->  SceneManager  |
///     threads   |   update  |                  |   query  |                |
///               |     |     +------------------+          +----------------+
///           +---+     |
///                     |
///                     +
///               thread barrier
///
/// This works well, as long as each Producer only requires the PropertyGraph to remain unchanged
///
class SceneManager { // TODO: update SceneManager docstring

    // friends
    friend struct SceneNodeParent;
    friend struct detail::SceneNodeChildHelper;
    template<typename, typename>
    friend struct SceneNodeChild;

    // forwards
    using SceneNodeBase = detail::SceneNodeBase;

    // types ---------------------------------------------------------------------------------------------------------//
public:
    NOTF_ACCESS_TYPES(SceneNode)

    //=========================================================================

    /// State of the SceneManager.
    class State {

        // methods ------------------------------------------------------------
    private:
        NOTF_ALLOW_MAKE_SMART_FROM_PRIVATE

        /// Constructor.
        /// @param layers   Layers that make up the State, ordered from front to back.
        State(std::vector<LayerPtr>&& layers) : m_layers(std::move(layers)) {}

    public:
        /// Layers that make up the State, ordered from front to back.
        const std::vector<LayerPtr> layers() const { return m_layers; }

        // fields -------------------------------------------------------------
    private:
        /// Layers that make up the State, ordered from front to back.
        std::vector<LayerPtr> m_layers;
    };
    using StatePtr = std::shared_ptr<State>;

    //=========================================================================
private:
    /// Empty class signifying that a SceneNode was deleted in the delta.
    struct DeletionDelta : public SceneNodeBase, public SceneNodeInterface<DeletionDelta> {

        /// Value Constructor.
        /// @param manager      SceneManager owning this SceneNode.
        DeletionDelta(SceneManager& manager) : SceneNodeBase(manager) {}

        /// Destructor.
        ~DeletionDelta() override;
    };

    // methods ------------------------------------------------------------------------------------------------------ //
public:
    /// Constructor.
    /// @param window   Window owning this SceneManager.
    SceneManager(Window& window);

    /// Destructor.
    /// Needed, because otherwise we'd have to include types contained in member unique_ptrs in the header.
    ~SceneManager();

    /// Current list of Layers, ordered from front to back.
    const std::vector<LayerPtr> layers() const { return m_current_state->layers(); }

    // state management -------------------------------------------------------

    /// Creates a new SceneManager::State
    /// @param layers   Layers that make up the new state, ordered from front to back.
    StatePtr create_state(std::vector<LayerPtr> layers)
    {
        return NOTF_MAKE_SHARED_FROM_PRIVATE(State, std::move(layers));
    }

    /// @{
    /// The current State of the SceneManager.
    StatePtr current_state() { return m_current_state; }
    const StatePtr current_state() const { return m_current_state; }
    /// @}

    /// Enters a given State.
    void enter_state(StatePtr state);

    // node hierarchy ---------------------------------------------------------
private:
    /// Creates a new SceneNode.
    /// @param args     Arguments to create the new node with.
    template<typename T, typename... Args, typename = std::enable_if_t<detail::is_valid_scene_node_type<T>()>>
    valid_ptr<T*> create_node(Args&&... args)
    {
        NOTF_ASSERT(m_mutex.is_locked_by_this_thread());
        std::unique_ptr<T> node = std::make_unique<T>(std::forward<Args>(args)...);
        valid_ptr<T*> id = node.get();
        m_nodes.emplace(std::make_pair(id, std::move(node)));
        return id;
    }

    /// Returns a SceneNode for reading without creating a new delta.
    /// @param node         SceneNode to read from.
    /// @param thread_id    Id of the thread calling this function (exposed for testability).
    /// @returns            The requested node or nullptr, if the node has a deletion delta.
    template<typename T>
    risky_ptr<const T*> read_node(valid_ptr<T*> node, const std::thread::id thread_id)
    {
        NOTF_ASSERT(m_mutex.is_locked_by_this_thread());
        if (!is_frozen() || is_render_thread(thread_id)) {
            return node; // direct access
        }

        // return the frozen node if there is no delta for it
        auto it = m_delta.find(node);
        if (it == m_delta.end()) {
            return node;
            /* TODO: AARRHH! We cannot return a node without holding the mutex as well ...
             * Actually, that's not a problem. Unlike with the PropertyManager, we don't need small-scoped locking of
             * Items. Instead, the lock on a SceneManager should be held every event, as long as the event executes.
             * This way, we can only render in between events. And the render manager doesn't modify the scene, reading
             * is okay. So it doesn't lock for long - just to freeze the graph. Then, the lock can be released and the
             * next event grabs it.
             * Of course, we have to make sure that a read-only node REALLY only reads, no mutability or such.
             * This requires correct use of constness - but I know that, and I will put it into the docs. And if a noob
             * C++ programmer ignores it, then there's nothing I can do.
             *
             * ACTUAL TO DO: document this properly and remove all existing locking of the SceneManager's mutex
             */
        }

        // return an existing modification delta
        const std::unique_ptr<SceneNodeBase>& delta = it->second;
        if (is_deletion_delta(delta.get())) {
            return nullptr;
        }
        return delta.get();
    }

    /// Returns a SceneNode for writing.
    /// Creates a new SceneNode if the graph is frozen.
    /// @param node     SceneNode to write into.
    /// @returns        The requested node or nullptr, if the node already has a deletion delta.
    template<typename T>
    risky_ptr<T*> write_node(valid_ptr<T*> node)
    {
        NOTF_ASSERT(m_mutex.is_locked_by_this_thread());
        if (!is_frozen()) {
            return node; // direct access
        }
        if (is_render_thread(std::this_thread::get_id())) {
            return nullptr; // do nothing if this is the render thread resolving the delta
        }

        // return an existing modification delta
        auto it = m_delta.find(node);
        if (it != m_delta.end()) {
            const std::unique_ptr<SceneNodeBase>& delta = it->second;
            if (is_deletion_delta(delta.get())) {
                return nullptr;
            }
            return static_cast<T*>(it->second.get());
        }

        // create a new modification delta
        std::unique_ptr<SceneNodeBase> new_delta = node->clone();
        T* result = static_cast<T*>(new_delta.get());
        m_delta.emplace(std::make_pair(node, std::move(new_delta)));
        return result;
    }

    /// Deletes a given SceneNode.
    /// Gives the Graph the chance to create a deletion delta for a deleted node.
    /// @param node         SceneNode to delete.
    /// @param thread_id    Id of the thread calling this function (exposed for testability).
    void delete_node(valid_ptr<SceneNodeBase*> node, const std::thread::id thread_id)
    {
        NOTF_ASSERT(m_mutex.is_locked_by_this_thread());

        // delete the property node straight away if the graph isn't frozen
        // or if this is the render thread resolving the delta
        if (!is_frozen() || is_render_thread(thread_id)) {
            auto it = m_nodes.find(node);
            NOTF_ASSERT_MSG(it != m_nodes.end(), "Cannot delete unknown SceneNode");
            m_nodes.erase(it);
            return;
        }

        // if the graph is frozen and the property is deleted, create a new deletion delta
        auto it = m_delta.find(node);
        if (it == m_delta.end()) {
            m_delta.emplace(std::make_pair(node, std::make_unique<DeletionDelta>(*this)));
            return;
        }

        // if there is already a modification delta in place, replace it with a deletion delta
        NOTF_ASSERT_MSG(!is_deletion_delta(it->second.get()), "Cannot delete the same SceneNode twice");
        std::unique_ptr<SceneNodeBase> deletion_delta = std::make_unique<DeletionDelta>(*this);
        it.value().swap(deletion_delta);
    }

    /// Checks if the PropertyGraph is currently frozen or not.
    bool is_frozen() const
    {
        NOTF_ASSERT(m_mutex.is_locked_by_this_thread());
        return m_render_thread != std::thread::id(0);
    }

    /// Checks if the calling thread is the current render thread.
    bool is_render_thread(const std::thread::id thread_id) const
    {
        NOTF_ASSERT(m_mutex.is_locked_by_this_thread());
        return thread_id == m_render_thread;
    }

    /// Checks if a given SceneNode is a DeletionDelta or not.
    static bool is_deletion_delta(const valid_ptr<SceneNodeBase*> ptr)
    {
        return (dynamic_cast<DeletionDelta*>(ptr.get()) != nullptr);
    }

    /// Freezes the Scene.
    /// All subsequent SceneNode modifications and removals will create Delta objects until the Delta is resolved.
    /// Does nothing iff the render thread tries to freeze the graph multiple times.
    /// @throws thread_error    If any but the render thread tries to freeze the manager.
    void freeze();

    /// Unfreezes the SceneManager and resolves all deltas.
    /// @throws thread_error    If any but the render thread tries to unfreeze the manager.
    void unfreeze();

    // fields --------------------------------------------------------------------------------------------------------//
private:
    /// Window owning this SceneManager.
    Window& m_window;

    /// Current State of the SceneManager.
    StatePtr m_current_state;

    /// All nodes managed by the graph.
    robin_map<valid_ptr<SceneNodeBase*>, std::unique_ptr<SceneNodeBase>, PointerHash<SceneNodeBase>> m_nodes;

    /// The current delta.
    robin_map<valid_ptr<SceneNodeBase*>, std::unique_ptr<SceneNodeBase>, PointerHash<SceneNodeBase>> m_delta;

    /// Mutex guarding the graph.
    mutable Mutex m_mutex;

    /// Thread id of the renderer thread, if it currently rendering.
    /// Also used as a flag whether the graph currently has a Delta or not.
    std::thread::id m_render_thread;
};

// ===================================================================================================================//

struct SceneNodeParent {

    using SceneNodeBase = detail::SceneNodeBase;

    // methods ----------------------------------------------------------------
protected:
    /// Constructor.
    /// @param node Parent SceneNode to manage.
    SceneNodeParent(valid_ptr<SceneNodeBase*> node) : m_node(node) {}

public:
    /// Mutable access to the parent node.
    /// May generate a new delta, if the SceneManager is currently frozen.
    valid_ptr<SceneNode*> get_mutable();

    /// Constant access to the parent node.
    /// Never generates a new delta.
    valid_ptr<const SceneNode*> get_const() const;

    // fields -----------------------------------------------------------------
private:
    /// The managed parent node.
    valid_ptr<SceneNodeBase*> m_node;
};

// ===================================================================================================================//

// forward
template<typename, typename>
struct SceneNodeChild;

namespace detail {

/// Helper object to attach static helper functions for SceneNodeChild<T> to.
struct SceneNodeChildHelper {
    template<typename, typename>
    friend struct notf::SceneNodeChild;

    /// Mutable access to the parent node.
    /// May generate a new delta, if the SceneManager is currently frozen.
    static valid_ptr<SceneNode*> get_mutable(valid_ptr<SceneNodeBase*> node);

    /// Constant access to the child node.
    /// Never generates a new delta.
    static valid_ptr<const SceneNode*> get_const(valid_ptr<SceneNodeBase*> node);
};

} // namespace detail

template<typename T, typename = std::enable_if_t<detail::is_valid_scene_node_type<T>()>>
struct SceneNodeChild {

    using SceneNodeBase = detail::SceneNodeBase;
    using Helper = detail::SceneNodeChildHelper;

    // methods ----------------------------------------------------------------
protected:
    /// Constructor.
    /// @param node ScenNode to manage.
    SceneNodeChild(valid_ptr<SceneNodeBase*> node) : m_node(node) {}

public:
    NOTF_NO_COPY_OR_ASSIGN(SceneNodeChild)

    /// Move Constructor.
    /// @param other    Property to move from.
    SceneNodeChild(SceneNodeChild&& other)
    {
        m_node = other.m_node;
        other.m_node = nullptr;
    }

    /// Destructor.
    ~SceneNodeChild()
    {
        if (m_node) {
            SceneManager& manager = m_node->manager();
            manager.delete_node(m_node, std::this_thread::get_id());
            m_node = nullptr;
        }
    }

    /// Mutable access to the child node.
    /// May generate a new delta, if the SceneManager is currently frozen.
    valid_ptr<T*> get_mutable() { return static_cast<T*>(Helper::get_mutable(m_node).get()); }

    /// Constant access to the child node.
    /// Never generates a new delta.
    valid_ptr<const T*> get_const() const { return static_cast<T*>(Helper::get_const(m_node).get()); }

    // fields -----------------------------------------------------------------
private:
    /// The managed child node.
    SceneNodeBase* m_node;
};

//====================================================================================================================//

template<>
class SceneManager::Access<SceneNode> {
    friend class SceneNode;

    /// Constructor.
    /// @param manager  The SceneNode to grant access.
    Access(valid_ptr<SceneNodeBase*> node) : m_manager(node->manager()) {}

    /// Adds a new child SceneNode to the SceneManager.
    /// @param args     Arguments to create the new node with.
    template<typename T, typename... Args, typename = std::enable_if_t<detail::is_valid_scene_node_type<T>()>>
    SceneNodeChild<T> add_child(Args&&... args)
    {
        return SceneNodeChild<T>(m_manager.create_node<T>(std::forward<Args>(args)...));
    }

    // fields --------------------------------------------------------------------------------------------------------//
private:
    /// The SceneManager to access.
    SceneManager& m_manager;
};

NOTF_CLOSE_NAMESPACE
