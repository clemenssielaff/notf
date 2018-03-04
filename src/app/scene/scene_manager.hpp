#pragma once

#include <unordered_map>
#include <vector>

#include "app/forwards.hpp"
#include "common/dag.hpp"
#include "common/forwards.hpp"
#include "common/id.hpp"

struct GLFWwindow;
NOTF_OPEN_NAMESPACE

using GraphicsProducerId = IdType<GraphicsProducer, size_t>; // TODO: id type forwards somewhere? in forwards.hpp?
using RenderTargetId     = IdType<RenderTarget, size_t>;

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
    /// Adds a new RenderTarget to the dependency list of a GraphicsProducer.
    /// @param producer     GraphicsProducer that depends on `target`.
    /// @param target       RenderTarget dependency for `producer`.
    void add(const GraphicsProducerId producer, const RenderTargetId target);

private:
    /// Sorts the given RenderTargets in an order which minimizes OpenGL state changes when rendering.
    /// @throws render_dependency_cycle If a GraphicsProducer is dependent on a RenderTarget that itself must produce.
    /// @returns Sorted RenderTargetIds.
    //    const std::vector<RenderTargetId>& sort();

    /// Resets the dependencies for a new calculation.
    void reset();

    // fields --------------------------------------------------------------------------------------------------------//
private:
    /// RenderTarget DAG.
    Dag<ushort> m_dag;

    /// Raw dependencies as reported by the GraphicsProducers.
    std::vector<std::pair<GraphicsProducerId, RenderTargetId>> m_dependencies;

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
/// Each Layer has a single GraphicsProducer (short: Producer) that define their content.
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
/// Some GraphicsProducer may require only properties in order to draw: the "smoke" FragmentProducer for example,
/// requires only the screen resolution and the time to update.
/// In that case, it is enough for the Application to update the PropertyManager with all of its accumulated updates
/// from various threads and then kick off the SceneManager of each Window.
///
///                     +
///                     |     (owned by Application)         (owned by Window)
///                     |              |                            |
///           +---+     |              v                            v
///               |     |     +------------------+          +----------------+
///     various   |   async   |                  |   sync   |                |
///               +----------->  PropertyManager +---------->  SceneManager  |
///     threads   |   update  |                  |   query  |                |
///               |     |     +------------------+          +----------------+
///           +---+     |
///                     |
///                     +
///               thread barrier
///
/// This works well, as long as each Producer only requires the PropertyManager to remain unchanged
///
class SceneManager {

    // types ---------------------------------------------------------------------------------------------------------//
public:
    NOTF_ACCESS_TYPES(Layer, GraphicsProducer, RenderTarget)

    /// Complete state of the Render Buffer.
    struct State {
        std::vector<LayerPtr> layers;
    };

    /// Ids for SceneManager states.
    using StateId = IdType<State, size_t>;

    // methods ------------------------------------------------------------------------------------------------------ //
protected:
    /// Constructor.
    /// @param window   GLFWwindow providing the OpenGL context.
    SceneManager(GLFWwindow* window);

public:
    /// Factory.
    /// @param window   GLFWwindow providing the OpenGL context.
    static SceneManagerPtr create(GLFWwindow* window);

    /// Destructor.
    /// Need, because otherwise we'd have to include types contained in member unique_ptrs in the header.
    ~SceneManager();

    ///@{
    /// Internal GraphicsContext.
    GraphicsContextPtr& graphics_context() { return m_graphics_context; }
    const GraphicsContextPtr& graphics_context() const { return m_graphics_context; }
    ///@}

    ///@{
    /// FontManager used to render text.
    FontManagerPtr& font_manager() { return m_font_manager; }
    const FontManagerPtr& font_manager() const { return m_font_manager; }
    ///@}

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

    /// Renders a single frame with the current State of the SceneManager.
    void render();

private:
    /// Registers a new GraphicsProducer.
    /// @throws runtime_error   If a GraphicsProducer with the same ID is already registered.
    void _register_new(GraphicsProducerPtr graphics_producer);

    /// Registers a new RenderTarget.
    /// @throws runtime_error   If a RenderTarget with the same ID is already registered.
    void _register_new(RenderTargetPtr render_target);

private:
    /// Generate the next available StateId.
    static StateId _next_id();

    // fields --------------------------------------------------------------------------------------------------------//
private:
    /// Internal GraphicsContext.
    GraphicsContextPtr m_graphics_context;

    /// FontManager used to render text.
    FontManagerPtr m_font_manager;

    detail::RenderDag m_dependencies;

    /// All States that the SceneManager knows.
    std::unordered_map<StateId, State> m_states;

    /// All GraphicsProducer that are registered with this SceneManager by their ID.
    std::unordered_map<GraphicsProducerId, GraphicsProducerPtr> m_graphics_producer;

    /// All RenderTargets that are registered with this RenderTargets by their ID.
    std::unordered_map<RenderTargetId, RenderTargetPtr> m_render_targets;

    /// The current state of the SceneManager.
    const State* m_state;

    /// The default State is assumed, whenever the SceneManager would otherwise be stateless.
    static const State s_default_state;
};

// ===================================================================================================================//

template<>
class SceneManager::Access<GraphicsProducer> {
    friend class GraphicsProducer;

    /// Constructor.
    /// @param render_manager   SceneManager to access.
    Access(SceneManager& render_manager) : m_render_manager(render_manager) {}

    /// Registers a new GraphicsProducer.
    /// @throws runtime_error   If a GraphicsProducer with the same ID is already registered.
    void register_new(GraphicsProducerPtr producer) { m_render_manager._register_new(std::move(producer)); }

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