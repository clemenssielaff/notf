#pragma once

#include "app/scene_graph.hpp"

NOTF_OPEN_NAMESPACE

// ================================================================================================================== //

/// Test Accessor providing test-related access functions to a SceneGraph.
/// The test::Harness accessors subvert about any safety guards in place and are only to be used for testing under
/// controlled circumstances (and only from a single thread)!
template<>
class access::_SceneGraph<test::Harness> {

    // methods ------------------------------------------------------------------------------------------------------ //
public:
    /// Constructor.
    /// @param graph    SceneGraph to access.
    _SceneGraph(SceneGraph& graph) : m_graph(graph) {}

    /// Factory method.
    static SceneGraphPtr create(valid_ptr<WindowPtr> window) { return SceneGraph::_create(std::move(window)); }

    /// Creates and returns a FreezeGuard that keeps the scene frozen while it is alive.
    /// @param thread_id    (pretend) Id of the freezing thread.
    SceneGraph::FreezeGuard freeze_guard(const std::thread::id thread_id = std::this_thread::get_id())
    {
        return m_graph._freeze_guard(thread_id);
    }

    /// Freezes the Scene.
    /// @param thread_id    (pretend) Id of the thread freezing the graph.
    void freeze(const std::thread::id thread_id = std::this_thread::get_id()) { m_graph._freeze(thread_id); }

    /// Unfreezes the Scene.
    /// @param thread_id    (pretend) Id of the thread unfreezing the graph.
    void unfreeze(const std::thread::id thread_id = std::this_thread::get_id()) { m_graph._unfreeze(thread_id); }

    /// The graph's event mutex.
    Mutex& event_mutex() { return m_graph.m_event_mutex; }

    /// Lets the caller pretend that this is the render thread.
    /// @param thread_id    (pretend) Id of the thread assumed to be the render thread by the Scene.
    void set_render_thread(const std::thread::id thread_id) { m_graph.m_freezing_thread = hash(thread_id); }

    // fields ------------------------------------------------------------------------------------------------------- //
private:
    /// SceneGraph to access.
    SceneGraph& m_graph;
};

NOTF_CLOSE_NAMESPACE
