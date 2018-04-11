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
        back = _add_child<LeafNode>();
        front = _add_child<LeafNode>();
    }

    void reverse() { back->stack_front(); }

    // fields -----------------------------------------------------------------
public:
    NodeHande<LeafNode> back;
    NodeHande<LeafNode> front;
};

//====================================================================================================================//

struct ThreeChildrenNode : public Scene::Node {

    /// Constructor.
    ///
    ThreeChildrenNode(const Token& token, Scene& scene, valid_ptr<Node*> parent) : Node(token, scene, parent)
    {
        back = _add_child<LeafNode>();
        center = _add_child<LeafNode>();
        front = _add_child<LeafNode>();
    }

    void reverse()
    {
        (*back).stack_front();
        (*center).stack_back();
    }

    // fields -----------------------------------------------------------------
public:
    NodeHande<LeafNode> back;
    NodeHande<LeafNode> center;
    NodeHande<LeafNode> front;
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

        std::thread::id event_thread_id = std::this_thread::get_id();
        std::thread::id render_thread_id(1);
#if 1
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

        SECTION("modifying nodes in a frozen scene will produce deltas that are resolved when unfrozen")
        {
            Scene::NodeHandle<TwoChildrenNode> a = scene.root().add_child<TwoChildrenNode>();

            REQUIRE(access.node_count() == 4);
            REQUIRE(access.child_container_count() == 4);
            REQUIRE(access.delta_count() == 0);

            { // frozen scope
                auto guard = access.freeze_guard(render_thread_id);

                REQUIRE(access.node_count() == 4);
                REQUIRE(access.child_container_count() == 4);
                REQUIRE(access.delta_count() == 0);

                // access from event thread
                REQUIRE(a->front->is_in_front());
                REQUIRE(a->back->is_in_back());

                // access from (pretend) render thread
                access.set_render_thread(event_thread_id);
                CHECK(a->front->is_in_front());
                CHECK(a->back->is_in_back());
                access.set_render_thread(render_thread_id);

                a->reverse();

                // access from event thread
                REQUIRE(a->front->is_in_back());
                REQUIRE(a->back->is_in_front());

                // access from (pretend) render thread
                access.set_render_thread(event_thread_id);
                CHECK(a->front->is_in_front());
                CHECK(a->back->is_in_back());
                access.set_render_thread(render_thread_id);

                REQUIRE(access.node_count() == 4);
                REQUIRE(access.child_container_count() == 4);
                REQUIRE(access.delta_count() == 1);
            }

            REQUIRE(a->front->is_in_back());
            REQUIRE(a->back->is_in_front());

            REQUIRE(access.node_count() == 4);
            REQUIRE(access.child_container_count() == 4);
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

            { // frozen scope
                auto guard = access.freeze_guard(render_thread_id);

                REQUIRE(access.node_count() == 12);
                REQUIRE(access.child_container_count() == 12);
                REQUIRE(access.delta_count() == 0);

                a->reverse();
                c->reverse();

                REQUIRE(access.node_count() == 12);
                REQUIRE(access.child_container_count() == 12);
                REQUIRE(access.delta_count() == 2); // a and c were modified

                { // delete c
                    Scene::NodeHandle<ThreeChildrenNode> dropped = std::move(c);
                }

                REQUIRE(access.node_count() == 12);
                REQUIRE(access.child_container_count() == 12);
                REQUIRE(access.delta_count() == 3);
            }

            REQUIRE(access.node_count() == 8);
            REQUIRE(access.child_container_count() == 8);
            REQUIRE(access.delta_count() == 0);
        }
//#else
        SECTION("nodes that are created and modified with a frozen scene will unfreeze with it")
        {
            REQUIRE(access.node_count() == 1);
            REQUIRE(access.child_container_count() == 1);
            REQUIRE(access.delta_count() == 0);

            {
                access.freeze(render_thread_id);

                Scene::NodeHandle<TwoChildrenNode> a = scene.root().add_child<TwoChildrenNode>();

                CHECK(a->front->is_in_front());
                CHECK(a->back->is_in_back());

                a->reverse();

                CHECK(a->front->is_in_back());
                CHECK(a->back->is_in_front());

                access.unfreeze(render_thread_id);

                REQUIRE(a->front->is_in_back());
                REQUIRE(a->back->is_in_front());
            }

            REQUIRE(access.node_count() == 1);
            REQUIRE(access.child_container_count() == 1);
            REQUIRE(access.delta_count() == 0);
        }
#endif
    }
}
