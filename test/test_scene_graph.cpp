#include "catch.hpp"

#include "test_node.hpp"
#include "test_scene.hpp"
#include "test_scene_graph.hpp"
#include "testenv.hpp"

NOTF_USING_NAMESPACE;

// ================================================================================================================== //

SCENARIO("a Scene can be set up and modified", "[app][scene_graph]")
{
    SceneGraphPtr scene_graph = SceneGraph::Access<test::Harness>::create(notf_window());
    std::shared_ptr<TestScene> scene_ptr = TestScene::create<TestScene>(scene_graph, "TestScene");
    TestScene& scene = *scene_ptr;

    SceneGraph::Access<test::Harness> graph_access(*scene_graph);
    Scene::Access<test::Harness> scene_access(scene);

    const std::thread::id event_thread_id = std::this_thread::get_id();
    const std::thread::id render_thread_id;

    //           TestScene
    //              |
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
    SECTION("SceneGraphs manage their scenes, nodes and -properties")
    {
        NOTF_MUTEX_GUARD(graph_access.event_mutex());

        NodeHandle<TestNode> a = scene.get_root().set_child<TestNode>("a");
        NodeHandle<TestNode> b = a->add_node<TestNode>("b");
        NodeHandle<TestNode> c = a->add_node<TestNode>("c");
        NodeHandle<TestNode> d = b->add_node<TestNode>("d");
        NodeHandle<TestNode> e = b->add_node<TestNode>("e");
        NodeHandle<TestNode> f = c->add_node<TestNode>("f");
        NodeHandle<TestNode> g = e->add_node<TestNode>("g");
        NodeHandle<TestNode> h = e->add_node<TestNode>("h");
        NodeHandle<TestNode> i = e->add_node<TestNode>("i");

        PropertyHandle<int> d1 = d->add_property<int>("d1", 1);

        REQUIRE((scene_graph->get_scene("TestScene") == scene_ptr) == true);
        REQUIRE(!scene_graph->get_scene("OtherScene"));

        REQUIRE(scene_graph->get_node<TestNode>("/TestScene/a/b/d") == d);
        REQUIRE(scene_graph->get_node<TestNode>("TestScene/a") == a);

        REQUIRE_THROWS_AS(scene_graph->get_node<TestNode>(Path()), Path::path_error);
        REQUIRE_THROWS_AS(scene_graph->get_node<TestNode>("/TestScene/a:property"), Path::path_error);
        REQUIRE_THROWS_AS(scene_graph->get_node<TestNode>("/OtherScene/a/b/d"), Path::path_error);
        REQUIRE_THROWS_AS(scene_graph->get_node<TestNode>("/TestScene"), Path::path_error);

        REQUIRE(scene.get_property<int>("/TestScene/a/b/d:d1") == d1);

        REQUIRE_THROWS_AS(scene_graph->get_property<int>(Path()), Path::path_error);
        REQUIRE_THROWS_AS(scene_graph->get_property<int>(Path("/:TestScene")), Path::path_error);
        REQUIRE_THROWS_AS(scene_graph->get_property<int>(Path("/TestScene/a/b/d")), Path::path_error);
        REQUIRE_THROWS_AS(scene_graph->get_property<int>(Path("/OtherScene/a/b/d:d1")), Path::path_error);
    }
}

// ================================================================================================================== //
