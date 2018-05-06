#pragma once

#include "app/scene.hpp"

NOTF_OPEN_NAMESPACE

//====================================================================================================================//

/// Test Accessor providing test-related access functions to a SceneGraph.
/// The test::Harness accessors subvert about any safety guards in place and are only to be used for testing under
/// controlled circumstances (and only from a single thread)!
template<>
class SceneGraph::Access<test::Harness> {

    // types ---------------------------------------------------------------------------------------------------------//
public:
    /// RAII object to make sure that a frozen scene is ALWAYS unfrozen again
    struct NOTF_NODISCARD FreezeGuard {

        NOTF_NO_COPY_OR_ASSIGN(FreezeGuard);
        NOTF_NO_HEAP_ALLOCATION(FreezeGuard);

        /// Constructor.
        /// @param graph        SceneGraph to freeze.
        /// @param thread_id    ID of the freezing thread, uses the calling thread by default. (exposed for testability)
        FreezeGuard(SceneGraph* graph, const std::thread::id thread_id = std::this_thread::get_id())
            : m_graph(graph), m_thread_id(thread_id)
        {
            if (!m_graph->_freeze(m_thread_id)) {
                m_graph = nullptr;
            }
        }

        /// Move constructor.
        /// @param other    FreezeGuard to move from.
        FreezeGuard(FreezeGuard&& other) : m_graph(other.m_graph) { other.m_graph = nullptr; }

        /// Destructor.
        ~FreezeGuard()
        {
            if (m_graph) { // don't unfreeze if this guard tried to double-freeze the scene
                m_graph->_unfreeze(m_thread_id);
            }
        }

        // fields -------------------------------------------------------------
    private:
        /// Scene to freeze.
        SceneGraph* m_graph;

        /// Id of the freezing thread.
        const std::thread::id m_thread_id;
    };

    // methods -------------------------------------------------------------------------------------------------------//
public:
    /// Constructor.
    /// @param graph    SceneGraph to access.
    Access(SceneGraph& graph) : m_graph(graph) {}

    /// Factory method.
    static SceneGraphPtr create(Window& window) { return SceneGraph::create(window); }

    /// Creates and returns a FreezeGuard that keeps the scene frozen while it is alive.
    /// @param thread_id    (pretend) Id of the freezing thread.
    FreezeGuard freeze_guard(const std::thread::id thread_id = std::this_thread::get_id())
    {
        return FreezeGuard(&m_graph, thread_id);
    }

    /// Freezes the Scene.
    /// @param thread_id    (pretend) Id of the thread freezing the graph.
    void freeze(const std::thread::id thread_id = std::this_thread::get_id()) { m_graph._freeze(thread_id); }

    /// Unfreezes the Scene.
    /// @param thread_id    (pretend) Id of the thread unfreezing the graph.
    void unfreeze(const std::thread::id thread_id = std::this_thread::get_id()) { m_graph._unfreeze(thread_id); }

    /// Lets the caller pretend that this is the render thread.
    /// @param thread_id    (pretend) Id of the thread assumed to be the render thread by the Scene.
    void set_render_thread(const std::thread::id thread_id) { m_graph.m_freezing_thread = hash(thread_id); }

    // fields --------------------------------------------------------------------------------------------------------//
private:
    /// SceneGraph to access.
    SceneGraph& m_graph;
};

//====================================================================================================================//

/// Test Accessor providing test-related access functions to a Scene.
/// The test::Harness accessors subvert about any safety guards in place and are only to be used for testing under
/// controlled circumstances (and only from a single thread)!
template<>
class Scene::Access<test::Harness> {

    // methods -------------------------------------------------------------------------------------------------------//
public:
    /// Constructor.
    /// @param scene    Scene to access.
    Access(Scene& scene) : m_scene(scene) {}

    /// Returns the number of nodes in the Scene.
    size_t node_count() { return m_scene.m_nodes.size(); }

    /// Returns the number of child container in the Scene.
    size_t child_container_count() { return m_scene.m_child_container.size(); }

    /// Returns the number of deltas in the Scene.
    size_t delta_count() { return m_scene.m_deltas.size(); }

    /// Returns the number of new nodes in the Scene.
    size_t deletions_count() { return m_scene.m_deletion_deltas.size(); }

    // fields --------------------------------------------------------------------------------------------------------//
private:
    /// Scene to access.
    Scene& m_scene;
};

NOTF_CLOSE_NAMESPACE
