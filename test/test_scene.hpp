#pragma once

#include "app/scene.hpp"

NOTF_OPEN_NAMESPACE

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

    /// Creates and returns a FreezeGuard that keeps the scene frozen while it is alive.
    /// @param thread_id    (pretend) Id of the freezing thread.
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
    /// @param thread_id    (pretend) Id of the thread freezing the graph.
    void freeze(const std::thread::id thread_id = std::this_thread::get_id()) { m_scene.freeze(thread_id); }

    /// Unfreezes the Scene.
    /// @param thread_id    (pretend) Id of the thread unfreezing the graph.
    void unfreeze(const std::thread::id thread_id = std::this_thread::get_id()) { m_scene.unfreeze(thread_id); }

    /// Lets the caller pretend that this is the render thread.
    /// @param thread_id    (pretend) Id of the thread assumed to be the render thread by the Scene.
    void set_render_thread(const std::thread::id thread_id) { m_scene.m_render_thread = thread_id; }

    // fields --------------------------------------------------------------------------------------------------------//
private:
    /// Scene to access.
    Scene& m_scene;
};

NOTF_CLOSE_NAMESPACE
