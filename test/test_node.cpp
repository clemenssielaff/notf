#include "catch.hpp"

#include "app/root_node.hpp"
#include "test_node.hpp"
#include "test_scene.hpp"
#include "testenv.hpp"

NOTF_USING_NAMESPACE

// ================================================================================================================== //

SCENARIO("creation / modification of a node hierarchy", "[app][node]")
{
    SceneGraphPtr scene_graph = SceneGraph::Access<test::Harness>::create(notf_window());
    std::shared_ptr<TestScene> scene_ptr = TestScene::create<TestScene>(scene_graph, "TestScene");
    TestScene& scene = *scene_ptr;

    SceneGraph::Access<test::Harness> graph_access(*scene_graph);
    Scene::Access<test::Harness> scene_access(scene);

    std::thread::id event_thread_id = std::this_thread::get_id();
    std::thread::id render_thread_id(1);

    SECTION("Child nodes cannot have the same name")
    {
        NOTF_MUTEX_GUARD(graph_access.event_mutex());

        NodeHandle<TestNode> a = scene.root().set_child<TestNode>();
        NodeHandle<TestNode> b = a->add_node<TestNode>();
        NodeHandle<TestNode> c = a->add_node<TestNode>();
        REQUIRE(scene_access.node_count() == 4);

        b->set_name("unique");
        REQUIRE(b->name() == "unique");

        c->set_name("unique");
        REQUIRE(c->name() != "unique");
    }

    SECTION("Node properties cannot have the same name")
    {
        NOTF_MUTEX_GUARD(graph_access.event_mutex());

        NodeHandle<TestNode> a = scene.root().set_child<TestNode>();
        REQUIRE(scene_access.node_count() == 2);

        a->add_property("unique", 1);
        REQUIRE_THROWS_AS(a->add_property("unique", 2), Path::not_unique_error);
    }

    SECTION("Nodes can be uniquely identified via their path")
    {
        NOTF_MUTEX_GUARD(graph_access.event_mutex());

        NodeHandle<TestNode> a = scene.root().set_child<TestNode>();
        NodeHandle<TestNode> b = a->add_node<TestNode>();
        NodeHandle<TestNode> c = b->add_node<TestNode>();
        NodeHandle<TestNode> d = c->add_node<TestNode>();
        a->set_name("a");
        b->set_name("b");
        c->set_name("c");
        d->set_name("d");

        const std::string path_a = a->path().to_string();
        REQUIRE(a->path() == Path("/TestScene/a"));
        REQUIRE(b->path() == Path("/TestScene/a/b"));
        REQUIRE(c->path() == Path("/TestScene/a/b/c"));
        REQUIRE(d->path() == Path("/TestScene/a/b/c/d"));

        SECTION("child paths update when their parent is renamed")
        {
            a->set_name("not_a");
            REQUIRE(b->path() == Path("/TestScene/not_a/b"));
        }
    }
}

// ================================================================================================================== //

TestNode::~TestNode() = default;

void TestNode::reverse_children()
{
    NOTF_MUTEX_GUARD(_hierarchy_mutex());
    _write_children().reverse();
}
