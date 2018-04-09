#include "catch.hpp"

#include "app/application.hpp"
#include "app/scene.hpp"
#include "app/scene_manager.hpp"
#include "app/window.hpp"
#include "testenv.hpp"

NOTF_USING_NAMESPACE

#pragma clang diagnostic ignored "-Wweak-vtables"

//====================================================================================================================//

struct TestScene : public Scene {
    TestScene(SceneManager& manager) : Scene(manager) {}
    void resize_view(Size2i) override {}
};

//====================================================================================================================//

struct LeafNode : public Scene::Node {
    /// Constructor.
    LeafNode(Scene& scene, valid_ptr<Node*> parent) : Node(scene, parent) {}
};

//====================================================================================================================//

struct TwoChildrenNode : public Scene::Node {

    /// Constructor.
    ///
    TwoChildrenNode(Scene& scene, valid_ptr<Node*> parent) : Node(scene, parent)
    {
        m_back = _add_child<LeafNode>(this->scene(), this);
        m_front = _add_child<LeafNode>(this->scene(), this);
    }

    void reverse() { m_back->stack_front(); }

    // fields -----------------------------------------------------------------
private:
    Scene::NodeHandle<LeafNode> m_back;
    Scene::NodeHandle<LeafNode> m_front;
};

//====================================================================================================================//

struct ThreeChildNode : public Scene::Node {

    /// Constructor.
    ThreeChildNode(Scene& scene, valid_ptr<Node*> parent) : Node(scene, parent)
    {
        m_center = _add_child<TwoChildrenNode>(this->scene(), this);
    }

    // fields -----------------------------------------------------------------
private:
    Scene::NodeHandle<TwoChildrenNode> m_center;
};

//====================================================================================================================//

// TODO: we say that you cannot create a node outside the `_add_child` method ... but you can. Use Tokens or something

SCENARIO("a Scene can be set up and modified", "[app], [scene]")
{
    GIVEN("an empty Scene")
    {
        SceneManager scene_manager(notf_window());
        TestScene scene(scene_manager);
        Scene::Access<test::Harness> access(scene);

        WHEN("you freeze an empty scene")
        {
            REQUIRE(access.node_count() == 1);            // root node
            REQUIRE(access.child_container_count() == 1); // root node
            REQUIRE(access.delta_count() == 0);

            {
                auto guard = access.freeze();
            }

            THEN("nothing changes")
            {
                REQUIRE(access.node_count() == 1);
                REQUIRE(access.child_container_count() == 1);
                REQUIRE(access.delta_count() == 0);
            }
        }

        WHEN("you create a few nodes")
        {
            {
                auto a = scene.root().add_child<TwoChildrenNode>(scene, &scene.root());
                auto b = scene.root().add_child<ThreeChildNode>(scene, &scene.root());

                REQUIRE(access.node_count() == 8);
                REQUIRE(access.child_container_count() == 8);
                REQUIRE(access.delta_count() == 0);

                AND_THEN("you modify them, they will not produce a delta")
                {
                    a->reverse();

                    REQUIRE(access.node_count() == 8);
                    REQUIRE(access.child_container_count() == 8);
                    REQUIRE(access.delta_count() == 0);
                }
            }

            AND_THEN("they are deleted again, the scene is empty")
            {
                REQUIRE(access.node_count() == 1);
                REQUIRE(access.child_container_count() == 1);
                REQUIRE(access.delta_count() == 0);
            }
        }
    }
}
