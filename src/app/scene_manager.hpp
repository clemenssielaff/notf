#pragma once

#include <vector>

#include "app/forwards.hpp"
#include "app/scene_node.hpp"
#include "common/hash.hpp"
#include "common/map.hpp"
#include "common/mutex.hpp"

NOTF_OPEN_NAMESPACE

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

    // types ---------------------------------------------------------------------------------------------------------//
public:
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

    /// Specialized hash to mix up the relative low entropy of pointers as key.
    struct NodeHash {
        size_t operator()(const SceneNode* node) const { return hash_mix(to_number(node)); }
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

    // fields --------------------------------------------------------------------------------------------------------//
private:
    /// Window owning this SceneManager.
    Window& m_window;

    /// Current State of the SceneManager.
    StatePtr m_current_state;

    /// All nodes managed by the graph.
    robin_map<SceneNode*, std::unique_ptr<SceneNode>, NodeHash> m_nodes;

    /// The current delta.
    robin_map<SceneNode*, std::unique_ptr<SceneNode>, NodeHash> m_delta;

    /// Mutex guarding the graph.
    mutable Mutex m_mutex;
};

NOTF_CLOSE_NAMESPACE
