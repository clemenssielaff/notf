#pragma once

#include "app/root_node.hpp"
#include "app/scene.hpp"

NOTF_OPEN_NAMESPACE

// ================================================================================================================== //

/// Test Accessor providing test-related access functions to a Scene.
/// The test::Harness accessors subvert about any safety guards in place and are only to be used for testing under
/// controlled circumstances (and only from a single thread)!
template<>
class access::_Scene<test::Harness> {

    // methods ------------------------------------------------------------------------------------------------------ //
public:
    /// Constructor.
    /// @param scene    Scene to access.
    _Scene(Scene& scene) : m_scene(scene) {}

    /// Returns the number of nodes in the Scene.
    size_t node_count() { return m_scene.count_nodes(); }

    /// Returns the number of deltas in the Scene.
    size_t delta_count() { return m_scene.m_frozen_children.size(); }

    // fields ------------------------------------------------------------------------------------------------------- //
private:
    /// Scene to access.
    Scene& m_scene;
};

// ================================================================================================================== //

struct TestScene : public Scene {
    TestScene(FactoryToken token, const valid_ptr<SceneGraphPtr>& graph, std::string name)
        : Scene(token, graph, std::move(name)), p_root_float(_root().create_property<float>("root_float", 0))
    {}

    ~TestScene() override;

    void _resize_view(Size2i) override {}

public: // fields
    PropertyHandle<float> p_root_float;
};

NOTF_CLOSE_NAMESPACE
