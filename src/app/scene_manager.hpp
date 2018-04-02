#pragma once

#include "app/forwards.hpp"
#include "common/hash.hpp"
#include "common/map.hpp"
#include "common/mutex.hpp"
#include "common/pointer.hpp"

NOTF_OPEN_NAMESPACE

//====================================================================================================================//

namespace detail {

/// Empty baseclass, used for empty SceneNode types like SceneManager::DeletionDelta.
class SceneNodeBase {
    // methods -------------------------------------------------------------------------------------------------------//
public:
    /// Value Constructor.
    /// @param manager      SceneManager owning this SceneNode.
    SceneNodeBase(SceneManager& manager) : m_manager(manager) {}

    /// Destructor.
    virtual ~SceneNodeBase();

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
class SceneManager {

    using SceneNodeBase = notf::detail::SceneNodeBase;

    friend struct SceneNodeParent;
    template<typename, typename>
    friend struct SceneNodeChild;

    // types ---------------------------------------------------------------------------------------------------------//
public:
    NOTF_ACCESS_TYPES(SceneNode)

    //=========================================================================

    /// Alias for checking if T is a valid SceneNode subtype.
    /// Subtypes must derive from SceneNode, must be copy constructible and not be const.
    template<typename T>
    using is_scene_node_type = typename std::conjunction<std::is_base_of<SceneNode, T>, std::is_copy_constructible<T>,
                                                         std::negation<std::is_const<T>>>::value;

    //=========================================================================

    /// State of the SceneManager.
    class State {

        // methods ------------------------------------------------------------
    private:
        NOTF_ALLOW_MAKE_SMART_FROM_PRIVATE

        /// Constructor.
        /// @param layers   Layers that make up the State.
        State(std::vector<LayerPtr>&& layers) : m_layers(std::move(layers)) {}

    public:
        /// Layers that make up the State.
        const std::vector<LayerPtr> layers() const { return m_layers; }

        // fields -------------------------------------------------------------
    private:
        std::vector<LayerPtr> m_layers;
    };
    using StatePtr = std::shared_ptr<State>;

    //=========================================================================
private:
    /// Empty class signifying that a SceneNode was deleted in the delta.
    struct DeletionDelta : public SceneNodeBase {
        /// Value Constructor.
        /// @param manager      SceneManager owning this SceneNode.
        DeletionDelta(SceneManager& manager) : SceneNodeBase(manager) {}

        /// Destructor.
        ~DeletionDelta() override;
    };

    // methods ------------------------------------------------------------------------------------------------------ //
public:
    /// Constructor.
    SceneManager(Window& window);

    /// Destructor.
    /// Needed, because otherwise we'd have to include types contained in member unique_ptrs in the header.
    ~SceneManager();

    /// Current list of Layers, ordered from front to back.
    const std::vector<LayerPtr> layers() const { return m_current_state->layers(); }

    // state management -------------------------------------------------------

    /// Creates a new SceneManager::State
    /// @param layers   Layers that make up the new state, ordered from front to back.
    template<typename... Layers>
    StatePtr create_state(Layers... layers)
    {
        return NOTF_MAKE_SHARED_FROM_PRIVATE(State, std::vector<LayerPtr>{layers...});
    }

    /// @{
    /// The current State of the SceneManager.
    StatePtr current_state() { return m_current_state; }
    const StatePtr current_state() const { return m_current_state; }
    /// @}

    /// Enters a State with a given ID.
    /// @throws resource_error  If no State with the given ID is known.
    void enter_state(StatePtr state);

private:
    /// Creates a new SceneNode.
    /// @param args     Arguments to create the new node with.
    template<typename T, typename... Args, typename = std::enable_if_t<is_scene_node_type<T>::value>>
    valid_ptr<T*> create_node(Args&&... args)
    {
        NOTF_ASSERT(m_mutex.is_locked_by_this_thread());
        std::unique_ptr<T> node = std::make_unique<T>(std::forward<Args>(args)...);
        valid_ptr<T*> ptr = node.get();
        m_nodes.emplace(std::make_pair(ptr, std::move(node)));
        return ptr;
    }

    /// Returns a SceneNode for reading without creating a new delta.
    /// @param node     SceneNodeto read from.
    /// @returns        The requested node or nullptr, if the node has a deletion delta.
    template<typename T, typename = std::enable_if_t<is_scene_node_type<T>::value>>
    risky_ptr<T*> read_node(valid_ptr<T*> node)
    {
        NOTF_ASSERT(m_mutex.is_locked_by_this_thread());
        if (!is_frozen() || is_render_thread()) {
            return node; // direct access
        }

        // return the frozen node if there is no delta for it
        auto it = m_delta.find(node);
        if (it == m_delta.end()) {
            return node;
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
    template<typename T, typename = std::enable_if_t<is_scene_node_type<T>::value>>
    risky_ptr<T*> write_node(valid_ptr<T*> node)
    {
        NOTF_ASSERT(m_mutex.is_locked_by_this_thread());
        if (!is_frozen()) {
            return node; // direct access
        }
        if (is_render_thread()) { // do nothing if this is the render thread resolving the delta
            return nullptr;
        }

        // return an existing modification delta
        auto it = m_delta.find(node);
        if (it != m_delta.end()) {
            const std::unique_ptr<SceneNodeBase>& delta = it->second;
            if (is_deletion_delta(delta.get())) {
                return nullptr;
            }
            return it->second->node();
        }

        // create a new modification delta
        std::unique_ptr<T> new_delta = std::make_unique<T>(*node);
        T* result = new_delta.get();
        m_delta.emplace(std::make_pair(node, std::move(new_delta)));
        return result;
    }

    /// Deletes a given SceneNode.
    /// Gives the Graph the chance to create a deletion delta for a deleted node.
    /// @param node     SceneNode to delete.
    void delete_node(valid_ptr<SceneNodeBase*> node)
    {
        NOTF_ASSERT(m_mutex.is_locked_by_this_thread());

        // delete the property node straight away if the graph isn't frozen
        // or if this is the render thread resolving the delta
        if (!is_frozen() || is_render_thread()) {
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
        return m_render_thread != std::thread::id();
    }

    /// Checks if the calling thread is the current render thread.
    bool is_render_thread() const
    {
        NOTF_ASSERT(m_mutex.is_locked_by_this_thread());
        return std::this_thread::get_id() == m_render_thread;
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

    // methods ----------------------------------------------------------------
protected:
    /// Constructor.
    /// @param node Parent SceneNode to manage.
    SceneNodeParent(valid_ptr<SceneNode*> node) : m_node(node) {}

public:
    valid_ptr<SceneNode*> get() { return m_node; } // TODO: LOOOOCKCKCKCKCKCK
    valid_ptr<const SceneNode*> get() const { return const_cast<SceneNodeParent*>(this)->get(); }

    // fields -----------------------------------------------------------------
private:
    valid_ptr<SceneNode*> m_node;
};

// ===================================================================================================================//

template<typename T, typename = std::enable_if_t<SceneManager::is_scene_node_type<T>::value>>
struct SceneNodeChild {

    using SceneNodeBase = detail::SceneNodeBase;

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
            manager.delete_node(m_node);
            m_node = nullptr;
        }
    }

    SceneNodeBase* get() { return m_node; } // TODO: LOOOOCKCKCKCKCKCK
    valid_ptr<const SceneNode*> get() const { return const_cast<SceneNodeChild*>(this)->get(); }

    // fields -----------------------------------------------------------------
private:
    SceneNodeBase* m_node;
};

//====================================================================================================================//

template<>
class SceneManager::Access<SceneNode> {
    friend class SceneNode;

    /// Constructor.
    /// @param manager  The SceneNode to grant access.
    Access(valid_ptr<SceneNodeBase*> node) : m_manager(node->manager()) {}

    /// Adds a new child SceneNode to the
    /// @param args     Arguments to create the new node with.
    template<typename T, typename... Args, typename = std::enable_if_t<SceneManager::is_scene_node_type<T>::value>>
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
