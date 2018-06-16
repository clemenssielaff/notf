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
    NodeHandle<TestNode> a, b, c, d, e, f, g, h, i;
    {
        NOTF_MUTEX_GUARD(graph_access.event_mutex());

        a = scene.root().set_child<TestNode>("a");
        b = a->add_node<TestNode>("b");
        c = a->add_node<TestNode>("c");
        d = b->add_node<TestNode>("d");
        e = b->add_node<TestNode>("e");
        f = c->add_node<TestNode>("f");
        g = e->add_node<TestNode>("g");
        h = e->add_node<TestNode>("h");
        i = e->add_node<TestNode>("i");

        REQUIRE(scene_access.node_count() == 10);
    }

    SECTION("Nodes form a tree hierarchy")
    {
        NOTF_MUTEX_GUARD(graph_access.event_mutex());

        REQUIRE(a->child_count() == 2);
        REQUIRE(e->child_count() == 3);
        REQUIRE(g->child_count() == 0);

        REQUIRE(b->child<TestNode>("d") == d);
        REQUIRE(e->child<TestNode>("i") == i);
        REQUIRE(!e->child<TestNode>("x").is_valid());

        REQUIRE(b->has_child("d"));
        REQUIRE(c->has_child("f"));
        REQUIRE(!a->has_child("d"));

        REQUIRE(b->child<TestNode>(0) == d);
        REQUIRE(b->child<TestNode>(1) == e);
        REQUIRE_THROWS_AS(b->child<TestNode>(2), NodeHandle<TestNode>::no_node_error);

        REQUIRE(f->has_ancestor(c));
        REQUIRE(f->has_ancestor(a));
        REQUIRE(!f->has_ancestor(b));
        REQUIRE(!f->has_ancestor(e));

        REQUIRE(g->common_ancestor(c) == a);
        REQUIRE(f->common_ancestor(e) == a);
        REQUIRE(e->common_ancestor(e) == e);
    }

    SECTION("Child nodes cannot have the same name")
    {
        NOTF_MUTEX_GUARD(graph_access.event_mutex());

        b->set_name("unique");
        REQUIRE(b->name() == "unique");

        c->set_name("unique");
        REQUIRE(c->name() == "unique1");

        b->set_name("007");
        REQUIRE(b->name() == "007");

        c->set_name("007");
        REQUIRE(c->name() == "0071");
    }

    SECTION("Nodes can have properties")
    {
        NOTF_MUTEX_GUARD(graph_access.event_mutex());

        REQUIRE_THROWS_AS(d->_BROKEN_create_property_after_finalized(), Node::node_finalized_error);

        PropertyHandle<int> d1 = d->add_property<int>("unique", 1);
        REQUIRE(d1.is_valid());
        REQUIRE(d1.get() == 1);
        REQUIRE_THROWS_AS(d->add_property<int>("unique", 2), Path::not_unique_error);

        REQUIRE(d->property<int>("unique") == d1);
        REQUIRE(b->property<int>(Path("./d:unique")) == d1);
        REQUIRE(b->property<int>(Path("/TestScene/a/b/d:unique")) == d1);
        REQUIRE(a->property<int>(Path("/TestScene/a/b/d:unique")) == d1);

        REQUIRE(!a->property<int>("nope").is_valid());
        REQUIRE(!a->property<int>(Path("/TestScene/a/b/d:doesn_exist")).is_valid());

        REQUIRE_THROWS_AS(d->property<int>(Path()), Path::path_error);
        REQUIRE_THROWS_AS(d->property<int>(Path("/TestScene/a/c:nope")), Path::path_error);
        REQUIRE_THROWS_AS(d->property<int>(Path("/TestScene/a/b/d")), Path::path_error);
    }

    SECTION("Nodes can be uniquely identified via their path")
    {
        NOTF_MUTEX_GUARD(graph_access.event_mutex());

        REQUIRE(a->path() == Path("/TestScene/a"));
        REQUIRE(b->path() == Path("/TestScene/a/b"));
        REQUIRE(c->path() == Path("/TestScene/a/c"));
        REQUIRE(d->path() == Path("/TestScene/a/b/d"));

        c->set_name("not_c");
        REQUIRE(f->path() == Path("/TestScene/a/not_c/f"));

        REQUIRE(a->child<TestNode>(Path("./b")) == b);
        REQUIRE(a->child<TestNode>(Path("./b/e")) == e);
        REQUIRE(a->child<TestNode>(Path("/TestScene/a/b/e/h")) == h);

        REQUIRE_THROWS_AS(b->child<TestNode>(Path()), Path::path_error);
        REQUIRE_THROWS_AS(b->child<TestNode>(Path("/TestScene/b/x")), Path::path_error);
        REQUIRE_THROWS_AS(b->child<TestNode>(Path("/TestScene/c/f")), Path::path_error);
        REQUIRE_THROWS_AS(b->child<TestNode>(Path("/TestScene/b:property")), Path::path_error);
    }

    SECTION("Nodes have a z-order that can be modified")
    {
        NOTF_MUTEX_GUARD(graph_access.event_mutex());

        REQUIRE(g->is_in_back());
        REQUIRE(h->is_before(g));
        REQUIRE(h->is_behind(i));
        REQUIRE(!h->is_before(h));
        REQUIRE(!h->is_behind(h));
        REQUIRE(i->is_in_front());

        g->stack_front();
        g->stack_front(); // unneccessary, but on purpose
        REQUIRE(g->is_in_front());
        REQUIRE(h->is_in_back());
        REQUIRE(i->is_before(h));
        REQUIRE(i->is_behind(g));

        i->stack_back();
        i->stack_back(); // unneccessary, but on purpose
        REQUIRE(g->is_in_front());
        REQUIRE(h->is_before(i));
        REQUIRE(h->is_behind(g));
        REQUIRE(i->is_in_back());

        NodeHandle<TestNode> j = e->add_node<TestNode>();
        REQUIRE(j->is_in_front());

        j->stack_behind(h);
        j->stack_behind(h); // unneccessary, but on purpose
        REQUIRE(g->is_in_front());
        REQUIRE(h->is_before(j));
        REQUIRE(j->is_behind(h));
        REQUIRE(i->is_in_back());

        i->stack_before(j);
        i->stack_before(j); // unneccessary, but on purpose
        REQUIRE(g->is_in_front());
        REQUIRE(h->is_behind(g));
        REQUIRE(i->is_before(j));
        REQUIRE(j->is_in_back());

        REQUIRE_THROWS_AS(a->is_before(j), Node::hierarchy_error);
    }

    SECTION("Nodes are represented by handles in user-space")
    {
        NOTF_MUTEX_GUARD(graph_access.event_mutex());

        REQUIRE(b.is_valid());

        NodeHandle<TestNode> x = scene.root().set_child<TestNode>("x");

        REQUIRE(!b.is_valid());
        REQUIRE_THROWS_AS(b->name(), NodeHandle<TestNode>::no_node_error);
    }

    SECTION("Nodes have access to all of their ancestors")
    {
        NOTF_MUTEX_GUARD(graph_access.event_mutex());

        REQUIRE(e->first_ancestor<TestNode>() == b);
        REQUIRE(e->first_ancestor<RootNode>() == a->parent());
    }
}

// ================================================================================================================== //

TestNode::~TestNode() = default;

void TestNode::reverse_children()
{
    NOTF_MUTEX_GUARD(_hierarchy_mutex());
    _write_children().reverse();
}
