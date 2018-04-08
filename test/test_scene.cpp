#include "catch.hpp"

#include "app/application.hpp"
#include "app/scene.hpp"
#include "app/scene_manager.hpp"
#include "app/window.hpp"
#include "testenv.hpp"

NOTF_USING_NAMESPACE

#pragma clang diagnostic ignored "-Wweak-vtables"

struct TestScene : public Scene {
    TestScene(SceneManager& manager) : Scene(manager) {}
    void resize_view(Size2i) override {}
};

struct LeafNode : public Scene::Node {
    LeafNode(Scene& scene, valid_ptr<Node*> parent) : Node(scene, parent) {}
};

struct TwoChildrenNode : public Scene::Node {
    TwoChildrenNode(Scene& scene, valid_ptr<Node*> parent) : Node(scene, parent)
    {
        m_left = _add_child<LeafNode>(this->scene(), this);
    }

    Scene::NodeHandle<LeafNode> m_left;
    Scene::NodeHandle<LeafNode> m_right;
};

SCENARIO("a Scene can be set up and modified", "[app], [scene]")
{
    GIVEN("an empty Scene")
    {
        SceneManager scene_manager(notf_window());
        TestScene scene(scene_manager);
        {
            {
                auto anode = scene.root().add_child<TwoChildrenNode>(scene, &scene.root());
            }
            int i = 3;
        }
    }
}
