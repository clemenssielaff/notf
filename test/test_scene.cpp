#include "catch.hpp"

#include "app/root_node.hpp"
#include "app/window.hpp"
#include "test_node.hpp"
#include "test_scene.hpp"
#include "testenv.hpp"

NOTF_USING_NAMESPACE

// ================================================================================================================== //

SCENARIO("a Scene can be set up and modified", "[app][scene]")
{
    SceneGraphPtr scene_graph = SceneGraph::Access<test::Harness>::create(notf_window());
    std::shared_ptr<TestScene> scene_ptr = TestScene::create<TestScene>(scene_graph, "TestScene");
    TestScene& scene = *scene_ptr;

    SceneGraph::Access<test::Harness> graph_access(*scene_graph);
    Scene::Access<test::Harness> scene_access(scene);

    const std::thread::id event_thread_id = std::this_thread::get_id();
    const std::thread::id render_thread_id;

    //              A
    //         +----+------+
    //         |           |
    //         B           C
    //     +---+---+       +
    //     |       |       |
    //     D       E       F
    //         +---+---+
    //         |   |   |
    //         G   H   I
    //
    SECTION("Scenes manage their nodes and -properties")
    {
        NOTF_MUTEX_GUARD(graph_access.event_mutex());

        NodeHandle<TestNode> a = scene.root().set_child<TestNode>("a");
        NodeHandle<TestNode> b = a->add_node<TestNode>("b");
        NodeHandle<TestNode> c = a->add_node<TestNode>("c");
        NodeHandle<TestNode> d = b->add_node<TestNode>("d");
        NodeHandle<TestNode> e = b->add_node<TestNode>("e");
        NodeHandle<TestNode> f = c->add_node<TestNode>("f");
        NodeHandle<TestNode> g = e->add_node<TestNode>("g");
        NodeHandle<TestNode> h = e->add_node<TestNode>("h");
        NodeHandle<TestNode> i = e->add_node<TestNode>("i");

        PropertyHandle<int> d1 = d->add_property<int>("d1", 1);

        REQUIRE(scene.node<TestNode>("/TestScene/a/b/d") == d);
        REQUIRE(scene.node<TestNode>("a") == a);
        REQUIRE(scene.node<TestNode>("a/b/d") == d);

        REQUIRE_THROWS_AS(scene.node<TestNode>(Path()), Path::path_error);
        REQUIRE_THROWS_AS(scene.node<TestNode>("/TestScene/a:property"), Path::path_error);
        REQUIRE_THROWS_AS(scene.node<TestNode>("/OtherScene/a/b/d"), Path::path_error);
        REQUIRE_THROWS_AS(scene.node<TestNode>("/TestScene"), Path::path_error);

        REQUIRE(scene.property<int>("/TestScene/a/b/d:d1") == d1);
        REQUIRE(scene.property<float>("root_float") == scene.p_root_float);

        REQUIRE_THROWS_AS(scene.property<int>(Path()), Path::path_error);
        REQUIRE_THROWS_AS(scene.property<int>(Path("/:TestScene")), Path::path_error);
        REQUIRE_THROWS_AS(scene.property<int>(Path("/TestScene/a/b/d")), Path::path_error);
        REQUIRE_THROWS_AS(scene.property<int>(Path("/OtherScene/a/b/d:d1")), Path::path_error);
    }

    SECTION("Scenes will always contain at least the root node")
    {
        NOTF_MUTEX_GUARD(graph_access.event_mutex());

        NodeHandle<TestNode> a = scene.root().set_child<TestNode>("a");
        NodeHandle<TestNode> b = a->add_node<TestNode>("b");
        NodeHandle<TestNode> c = a->add_node<TestNode>("c");
        REQUIRE(scene_access.node_count() == 4);

        scene.clear();
        REQUIRE(scene_access.node_count() == 1);
    }

    SECTION("Scenes must have an unique name")
    {
        REQUIRE_THROWS_AS(TestScene::create<TestScene>(scene_graph, "TestScene"), Scene::scene_name_error);
    }

    SECTION("freezing an empty scene has no effect")
    {
        { // event thread
            NOTF_MUTEX_GUARD(graph_access.event_mutex());
            REQUIRE(scene_access.node_count() == 1);
            REQUIRE(scene_access.delta_count() == 0);
        }
        { // render thread
            auto guard = graph_access.freeze_guard();
        }
        { // event thread
            NOTF_MUTEX_GUARD(graph_access.event_mutex());
            REQUIRE(scene_access.node_count() == 1);
            REQUIRE(scene_access.delta_count() == 0);
        }
    }

    SECTION("creating, modifying and deleting without freezing produces no deltas")
    {
        { // event thread
            NOTF_MUTEX_GUARD(graph_access.event_mutex());

            NodeHandle<TestNode> first_node = scene.root().set_child<TestNode>(); // + 1
            NodeHandle<TestNode> a = first_node->add_subtree(2);                  // + 3
            NodeHandle<TestNode> b = first_node->add_subtree(3);                  // + 4
            NodeHandle<TestNode> c = first_node->add_subtree(3);                  // + 4 + root

            REQUIRE(scene_access.node_count() == 13);
            REQUIRE(scene_access.delta_count() == 0);

            a->reverse_children();
            c->reverse_children();

            REQUIRE(scene_access.node_count() == 13);
            REQUIRE(scene_access.delta_count() == 0);

            first_node->clear();

            REQUIRE(scene_access.node_count() == 2);
            REQUIRE(scene_access.delta_count() == 0);
        }
    }
    SECTION("modifying nodes in a frozen scene will produce deltas that are resolved when unfrozen")
    {
        NodeHandle<TestNode> node, back, front;

        { // event thread
            NOTF_MUTEX_GUARD(graph_access.event_mutex());
            NodeHandle<TestNode> first = scene.root().set_child<TestNode>();
            node = first->add_subtree(2);

            REQUIRE(scene_access.node_count() == 5);
            REQUIRE(scene_access.delta_count() == 0);
        }
        { // frozen scope
            auto guard = graph_access.freeze_guard(render_thread_id);
            back = node->child<TestNode>(0);
            front = node->child<TestNode>(1);

            { // event thread
                NOTF_MUTEX_GUARD(graph_access.event_mutex());
                REQUIRE(scene_access.node_count() == 5);
                REQUIRE(scene_access.delta_count() == 0);

                REQUIRE(front->is_in_front());
                REQUIRE(back->is_in_back());
            }
            { // render thread
                NOTF_MUTEX_GUARD(graph_access.event_mutex());
                graph_access.set_render_thread(event_thread_id);
                CHECK(front->is_in_front());
                CHECK(back->is_in_back());
                graph_access.set_render_thread(render_thread_id);
            }
            { // event thread
                NOTF_MUTEX_GUARD(graph_access.event_mutex());
                node->reverse_children();
                REQUIRE(front->is_in_back());
                REQUIRE(back->is_in_front());
            }
            { // render thread
                NOTF_MUTEX_GUARD(graph_access.event_mutex());
                graph_access.set_render_thread(event_thread_id);
                CHECK(front->is_in_front());
                CHECK(back->is_in_back());
                graph_access.set_render_thread(render_thread_id);

                REQUIRE(scene_access.node_count() == 5);
                REQUIRE(scene_access.delta_count() == 1);
            }
        } // end of frozen scope
        { // event thread
            NOTF_MUTEX_GUARD(graph_access.event_mutex());
            REQUIRE(front->is_in_back());
            REQUIRE(back->is_in_front());

            REQUIRE(scene_access.node_count() == 5);
            REQUIRE(scene_access.delta_count() == 0);
        }
    }

    SECTION("deleting nodes from a frozen scene will produce deltas that are resolved when unfrozen")
    {
        NodeHandle<TestNode> first_node;
        NodeHandle<TestNode> a;
        NodeHandle<TestNode> b;
        NodeHandle<TestNode> c;

        { // event thread
            NOTF_MUTEX_GUARD(graph_access.event_mutex());
            first_node = scene.root().set_child<TestNode>();
            a = first_node->add_subtree(2);
            b = first_node->add_subtree(3);
            c = first_node->add_subtree(3);

            REQUIRE(scene_access.node_count() == 13);
            REQUIRE(scene_access.delta_count() == 0);
        }
        { // frozen scope
            auto guard = graph_access.freeze_guard(render_thread_id);

            { // event thread
                NOTF_MUTEX_GUARD(graph_access.event_mutex());
                REQUIRE(scene_access.node_count() == 13);
                REQUIRE(scene_access.delta_count() == 0);

                a->reverse_children();
                c->reverse_children();

                REQUIRE(scene_access.node_count() == 13);
                REQUIRE(scene_access.delta_count() == 2); // a and c were modified

                first_node->remove_child(c);
            }
            { // // the render thread still sees the original 13 nodes
                NOTF_MUTEX_GUARD(graph_access.event_mutex());
                graph_access.set_render_thread(event_thread_id);
                REQUIRE(scene_access.node_count() == 13);
                graph_access.set_render_thread(render_thread_id);
            }
            { // the event handler already has the updated number of 9 nodes
                NOTF_MUTEX_GUARD(graph_access.event_mutex());
                REQUIRE(scene_access.node_count() == 9);
                REQUIRE(scene_access.delta_count() == 3);
            }
        } // end of frozen scope
    }

    SECTION("nodes that are created and modified with a frozen scene will unfreeze with it")
    {
        NodeHandle<TestNode> first_node, node, back, front;

        { // event thread
            NOTF_MUTEX_GUARD(graph_access.event_mutex());
            first_node = scene.root().set_child<TestNode>();
        }

        {
            graph_access.freeze(render_thread_id);

            { // event thread
                NOTF_MUTEX_GUARD(graph_access.event_mutex());
                node = first_node->add_subtree(2);
                back = node->child<TestNode>(0);
                front = node->child<TestNode>(1);

                CHECK(front->is_in_front());
                CHECK(back->is_in_back());

                node->reverse_children();

                CHECK(front->is_in_back());
                CHECK(back->is_in_front());
            }

            graph_access.unfreeze(render_thread_id);

            { // event thread
                NOTF_MUTEX_GUARD(graph_access.event_mutex());
                REQUIRE(scene_access.delta_count() == 0);

                REQUIRE(front->is_in_back());
                REQUIRE(back->is_in_front());

                REQUIRE(scene_access.node_count() == 5);
            }
        }
    }

    SECTION("nodes that are create & removed while frozen do not affect the scene when unfrozen again")
    {
        NodeHandle<TestNode> first_node;

        { // event thread
            NOTF_MUTEX_GUARD(graph_access.event_mutex());
            first_node = scene.root().set_child<TestNode>();

            REQUIRE(scene_access.node_count() == 2);
            REQUIRE(scene_access.delta_count() == 0);
        }
        { // frozen scope
            auto guard = graph_access.freeze_guard(render_thread_id);

            { // event thread
                NOTF_MUTEX_GUARD(graph_access.event_mutex());

                NodeHandle<TestNode> a = first_node->add_subtree(2);
                NodeHandle<TestNode> b = first_node->add_subtree(3);
                NodeHandle<TestNode> c = first_node->add_subtree(3);
                NodeHandle<TestNode> d = first_node->add_node<TestNode>();

                first_node->clear();

                // creating adding children in the constructor doesn't count towards the delta
                REQUIRE(scene_access.delta_count() == 1);
            }
        } // end of frozen scope
        { // event thread
            NOTF_MUTEX_GUARD(graph_access.event_mutex());
            REQUIRE(scene_access.node_count() == 2);
            REQUIRE(scene_access.delta_count() == 0);
        }
    }
}

// ================================================================================================================== //

TestScene::~TestScene() {}
