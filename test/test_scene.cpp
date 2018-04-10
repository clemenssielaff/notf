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
    LeafNode(const Token& token, Scene& scene, valid_ptr<Node*> parent) : Node(token, scene, parent) {}
};

//====================================================================================================================//

struct TwoChildrenNode : public Scene::Node {

    /// Constructor.
    ///
    TwoChildrenNode(const Token& token, Scene& scene, valid_ptr<Node*> parent) : Node(token, scene, parent)
    {
        m_back = _add_child<LeafNode>();
        m_front = _add_child<LeafNode>();
    }

    void reverse() { m_back->stack_front(); }

    // fields -----------------------------------------------------------------
private:
    NodeHande<LeafNode> m_back;
    NodeHande<LeafNode> m_front;
};

//====================================================================================================================//

struct ThreeChildrenNode : public Scene::Node {

    /// Constructor.
    ///
    ThreeChildrenNode(const Token& token, Scene& scene, valid_ptr<Node*> parent) : Node(token, scene, parent)
    {
        m_back = _add_child<LeafNode>();
        m_center = _add_child<LeafNode>();
        m_front = _add_child<LeafNode>();
    }

    void reverse()
    {
        (*m_back).stack_front();
        (*m_center).stack_back();
    }

    // fields -----------------------------------------------------------------
private:
    NodeHande<LeafNode> m_back;
    NodeHande<LeafNode> m_center;
    NodeHande<LeafNode> m_front;
};

//====================================================================================================================//

struct SplitNode : public Scene::Node {

    /// Constructor.
    SplitNode(const Token& token, Scene& scene, valid_ptr<Node*> parent) : Node(token, scene, parent)
    {
        m_center = _add_child<TwoChildrenNode>();
    }

    // fields -----------------------------------------------------------------
private:
    Scene::NodeHandle<TwoChildrenNode> m_center;
};

//====================================================================================================================//

SCENARIO("a Scene can be set up and modified", "[app], [scene]")
{
    GIVEN("an empty Scene")
    {
        SceneManager scene_manager(notf_window());
        TestScene scene(scene_manager);
        Scene::Access<test::Harness> access(scene);

        std::thread::id reader_thread(1);
        std::thread::id render_thread(2);

        SECTION("freezing an empty scene has no effect")
        {
            REQUIRE(access.node_count() == 1);            // root node
            REQUIRE(access.child_container_count() == 1); // root node
            REQUIRE(access.delta_count() == 0);

            {
                auto guard = access.freeze_guard();
            }

            REQUIRE(access.node_count() == 1);
            REQUIRE(access.child_container_count() == 1);
            REQUIRE(access.delta_count() == 0);
        }

        SECTION("creating, modifying and deleting without freezing produces no deltas")
        {
            {
                Scene::NodeHandle<TwoChildrenNode> a = scene.root().add_child<TwoChildrenNode>();
                Scene::NodeHandle<SplitNode> b = scene.root().add_child<SplitNode>();
                Scene::NodeHandle<ThreeChildrenNode> c = scene.root().add_child<ThreeChildrenNode>();

                REQUIRE(access.node_count() == 12);
                REQUIRE(access.child_container_count() == 12);
                REQUIRE(access.delta_count() == 0);

                a->reverse();
                c->reverse();

                REQUIRE(access.node_count() == 12);
                REQUIRE(access.child_container_count() == 12);
                REQUIRE(access.delta_count() == 0);
            }

            REQUIRE(access.node_count() == 1);
            REQUIRE(access.child_container_count() == 1);
            REQUIRE(access.delta_count() == 0);
        }

        SECTION("deleting nodes from a frozen scene will produce deltas that are resolved when unfrozen")
        {
            Scene::NodeHandle<TwoChildrenNode> a = scene.root().add_child<TwoChildrenNode>();
            Scene::NodeHandle<SplitNode> b = scene.root().add_child<SplitNode>();
            Scene::NodeHandle<ThreeChildrenNode> c = scene.root().add_child<ThreeChildrenNode>();

            REQUIRE(access.node_count() == 12);
            REQUIRE(access.child_container_count() == 12);
            REQUIRE(access.delta_count() == 0);

            a->reverse();
            c->reverse();

            { // frozen scope
                auto guard = access.freeze_guard(render_thread);

                REQUIRE(access.node_count() == 12);
                REQUIRE(access.child_container_count() == 12);
                REQUIRE(access.delta_count() == 0);

                { // delete c
                    Scene::NodeHandle<ThreeChildrenNode> dropped = std::move(c);
                }

                REQUIRE(access.node_count() == 12);
                REQUIRE(access.child_container_count() == 12);
                REQUIRE(access.delta_count() == 1);
            }

            REQUIRE(access.node_count() == 8);
            REQUIRE(access.child_container_count() == 8);
            REQUIRE(access.delta_count() == 0);
        }
    }
}
