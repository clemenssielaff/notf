#pragma once

#include <unordered_map>
#include <vector>

#include "app/ids.hpp"
#include "common/dag.hpp"
#include "common/id.hpp"
#include "common/size2.hpp"

NOTF_OPEN_NAMESPACE

// ===================================================================================================================//

namespace detail {

/// Helper class used by the SceneManager to make sure that each Producer is called after the RenderTargets that they
/// depend on are clean, and that RenderTargets are cleaned using the smallest number of OpenGL state changes possible.
class RenderDag {
    friend class notf::SceneManager;

    // methods -------------------------------------------------------------------------------------------------------//
private:
    /// Default constructor.
    RenderDag() : m_dag(), m_dependencies(), m_new_hash(0), m_last_hash(0) {}

public:
    /// Adds a new RenderTarget to the dependency list of a Renderer.
    /// @param producer     Renderer that depends on `target`.
    /// @param target       RenderTarget dependency for `producer`.
    void add(const RendererId producer, const RenderTargetId target);

private:
    /// Sorts the given RenderTargets in an order which minimizes OpenGL state changes when rendering.
    /// @throws render_dependency_cycle If a Renderer is dependent on a RenderTarget that itself must produce.
    /// @returns Sorted RenderTargetIds.
    //    const std::vector<RenderTargetId>& sort();

    /// Resets the dependencies for a new calculation.
    void reset();

    // fields --------------------------------------------------------------------------------------------------------//
private:
    /// RenderTarget DAG.
    Dag<ushort> m_dag;

    /// Raw dependencies as reported by the Renderers.
    std::vector<std::pair<RendererId, RenderTargetId>> m_dependencies;

    /// We expect the render layout to change only occasionally. Most of the time, it will be the same as it was last
    /// frame. In order to avoid unnecessary re-sorting of the RenderTargets, we hash the order in which the Producers
    /// and Targets were reported. If they are the same, we don't need to sort them again.
    size_t m_new_hash;

    /// See `m_new_hash` for details.
    size_t m_last_hash;
};

} // namespace detail

// ===================================================================================================================//

///
///
/// State
/// =====
///
/// The SceneManager has a STATE that defines how to render a frame.
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
///      Producer1  Producer2              Producer3          |
///                    +----------+     +------+----+       depends
///                            RenderTarget1        |         on
///                                 |               |         |
///                             Producer4           |         |
///                                 +--------+      |         |
///                                        RenderTarget2      V
///                                              |
///                                          Producer5
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
///               +----------->  PropertyGraph +---------->  SceneManager  |
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

    // types ---------------------------------------------------------------------------------------------------------//
public:
    NOTF_ACCESS_TYPES(Renderer, RenderTarget)

    /// Complete state of the Render Buffer.
    struct State {
        std::vector<LayerPtr> layers;

        /// Checks if this State is valid or not.
        bool is_valid() const { return this != &SceneManager::s_default_state; }
    };

    /// Ids for SceneManager states.
    using StateId = IdType<State, size_t>;

    // methods ------------------------------------------------------------------------------------------------------ //
public:
    /// Constructor.
    SceneManager();

    /// Destructor.
    /// Need, because otherwise we'd have to include types contained in member unique_ptrs in the header.
    ~SceneManager();

    // state management -------------------------------------------------------

    /// Adds a new State to the SceneManager.
    /// @param state    New State to add.
    /// @returns        Id of the new state.
    StateId add_state(State&& state);

    /// Checks if the Manager knows about a State with the given ID.
    bool has_state(const StateId id) const { return m_states.count(id) != 0; }

    /// Read-only access to the current State of the SceneManager.
    const State& current_state() const { return *m_state; }

    /// Read-only access to a State by its ID.
    /// @throws resource_error  If no State with the given ID is known.
    const State& state(const StateId id) const;

    /// Enters a State with a given ID.
    /// @throws resource_error  If no State with the given ID is known.
    void enter_state(const StateId id);

    /// Removes the State with the given ID.
    /// If the State to remove is the curren State, the SceneManager will fall back to the default state.
    /// @throws resource_error  If no State with the given ID is known.
    void remove_state(const StateId id);

    // event propagation ------------------------------------------------------

    /// Called when the a mouse button is pressed or released, the mouse is moved inside the Window, the mouse wheel
    /// scrolled or the cursor enters or exists the client area of a Window.
    /// @param event    MouseEvent.
    /// @returns        True iff the Scene handled the event and it doesn't need to be propagated further.
    void propagate(MouseEvent&& event);

    /// Called when a key is pressed, repeated or released.
    /// @param event    KeyEvent.
    /// @returns        True iff the Scene handled the event and it doesn't need to be propagated further.
    void propagate(KeyEvent&& event);

    /// Called when an unicode code point is generated.
    /// @param event    CharEvent.
    /// @returns        True iff the Scene handled the event and it doesn't need to be propagated further.
    void propagate(CharEvent&& event);

    /// Called when the cursor enters or exits the Window's client area or the window is about to be closed.
    /// @param event    WindowEvent.
    /// @returns        True iff the Scene handled the event and it doesn't need to be propagated further.
    void propagate(WindowEvent&& event);

    /// Called when the Window containing the Scene is resized.
    /// @param size     New size.
    void resize(Size2i size);

private:
    /// Registers a new Renderer.
    /// @throws runtime_error   If a Renderer with the same ID is already registered.
    void _register_new(RendererPtr graphics_producer);

    /// Registers a new RenderTarget.
    /// @throws runtime_error   If a RenderTarget with the same ID is already registered.
    void _register_new(RenderTargetPtr render_target);

    /// Generate the next available StateId.
    static StateId _next_id();

    // fields --------------------------------------------------------------------------------------------------------//
private:
    /// The current state of the SceneManager.
    const State* m_state;

    detail::RenderDag m_dependencies;

    /// All States that the SceneManager knows.
    std::unordered_map<StateId, State> m_states;

    /// All Renderer that are registered with this SceneManager by their ID.
    std::unordered_map<RendererId, RendererPtr> m_graphics_producer;

    /// All RenderTargets that are registered with this RenderTargets by their ID.
    std::unordered_map<RenderTargetId, RenderTargetPtr> m_render_targets;

    /// The default State is assumed, whenever the SceneManager would otherwise be stateless.
    static const State s_default_state;
};

// ===================================================================================================================//

template<>
class SceneManager::Access<Renderer> {
    friend class Renderer;

    /// Constructor.
    /// @param render_manager   SceneManager to access.
    Access(SceneManager& render_manager) : m_render_manager(render_manager) {}

    /// Registers a new Renderer.
    /// @throws runtime_error   If a Renderer with the same ID is already registered.
    void register_new(RendererPtr producer) { m_render_manager._register_new(std::move(producer)); }

    /// The SceneManager to access.
    SceneManager& m_render_manager;
};

template<>
class SceneManager::Access<RenderTarget> {
    friend class RenderTarget;

    /// Constructor.
    /// @param render_manager   SceneManager to access.
    Access(SceneManager& render_manager) : m_render_manager(render_manager) {}

    /// Registers a new RenderTarget.
    /// @throws runtime_error   If a RenderTarget with the same ID is already registered.
    void register_new(RenderTargetPtr render_target) { m_render_manager._register_new(std::move(render_target)); }

    /// The SceneManager to access.
    SceneManager& m_render_manager;
};

NOTF_CLOSE_NAMESPACE
