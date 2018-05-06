#include "catch.hpp"

#include "app/window.hpp"
#include "test_scene.hpp"
#include "testenv.hpp"

NOTF_USING_NAMESPACE

#pragma clang diagnostic ignored "-Wweak-vtables"

//====================================================================================================================//

struct TestScene : public Scene {
    TestScene(const FactoryToken& token, const valid_ptr<SceneGraphPtr>& graph) : Scene(token, graph) {}
    void _resize_view(Size2i) override {}
};

//====================================================================================================================//

struct LeafNode : public SceneNode {
    /// Constructor.
    LeafNode(const FactoryToken& token, Scene& scene, valid_ptr<SceneNode*> parent) : SceneNode(token, scene, parent) {}
};

//====================================================================================================================//

struct TwoChildrenNode : public SceneNode {

    /// Constructor.
    ///
    TwoChildrenNode(const FactoryToken& token, Scene& scene, valid_ptr<SceneNode*> parent)
        : SceneNode(token, scene, parent)
    {
        back = _add_child<LeafNode>();
        front = _add_child<LeafNode>();
    }

    void reverse() { back->stack_front(); }

    // fields -----------------------------------------------------------------
public:
    NodeHandle<LeafNode> back;
    NodeHandle<LeafNode> front;
};

//====================================================================================================================//

struct ThreeChildrenNode : public SceneNode {

    /// Constructor.
    ///
    ThreeChildrenNode(const FactoryToken& token, Scene& scene, valid_ptr<SceneNode*> parent)
        : SceneNode(token, scene, parent)
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
    NodeHandle<LeafNode> back;
    NodeHandle<LeafNode> center;
    NodeHandle<LeafNode> front;
};

//====================================================================================================================//

struct SplitNode : public SceneNode {

    /// Constructor.
    SplitNode(const FactoryToken& token, Scene& scene, valid_ptr<SceneNode*> parent) : SceneNode(token, scene, parent)
    {
        m_center = _add_child<TwoChildrenNode>();
    }

    // fields -----------------------------------------------------------------
private:
    NodeHandle<TwoChildrenNode> m_center;
};

//====================================================================================================================//

SCENARIO("a Scene can be set up and modified", "[app], [scene]")
{
    SceneGraphPtr scene_graph = SceneGraph::Access<test::Harness>::create(notf_window());
    std::shared_ptr<TestScene> scene_ptr = TestScene::create<TestScene>(scene_graph);
    TestScene& scene = *scene_ptr;
    SceneGraph::Access<test::Harness> graph_access(*scene_graph);
    Scene::Access<test::Harness> scene_access(scene);

    std::thread::id event_thread_id = std::this_thread::get_id();
    std::thread::id render_thread_id(1);
#if 1
    SECTION("freezing an empty scene has no effect")
    {
        REQUIRE(scene_access.node_count() == 1);            // root node
        REQUIRE(scene_access.child_container_count() == 1); // root node
        REQUIRE(scene_access.delta_count() == 0);

        {
            auto guard = graph_access.freeze_guard();
        }

        REQUIRE(scene_access.node_count() == 1);
        REQUIRE(scene_access.child_container_count() == 1);
        REQUIRE(scene_access.delta_count() == 0);
    }

    SECTION("creating, modifying and deleting without freezing produces no deltas")
    {
        {
            NodeHandle<TwoChildrenNode> a = scene.root().add_child<TwoChildrenNode>();
            NodeHandle<SplitNode> b = scene.root().add_child<SplitNode>();
            NodeHandle<ThreeChildrenNode> c = scene.root().add_child<ThreeChildrenNode>();

            REQUIRE(scene_access.node_count() == 12);
            REQUIRE(scene_access.child_container_count() == 12);
            REQUIRE(scene_access.delta_count() == 0);

            a->reverse();
            c->reverse();

            REQUIRE(scene_access.node_count() == 12);
            REQUIRE(scene_access.child_container_count() == 12);
            REQUIRE(scene_access.delta_count() == 0);
        }

        REQUIRE(scene_access.node_count() == 1);
        REQUIRE(scene_access.child_container_count() == 1);
        REQUIRE(scene_access.delta_count() == 0);
    }

    SECTION("modifying nodes in a frozen scene will produce deltas that are resolved when unfrozen")
    {
        NodeHandle<TwoChildrenNode> a = scene.root().add_child<TwoChildrenNode>();

        REQUIRE(scene_access.node_count() == 4);
        REQUIRE(scene_access.child_container_count() == 4);
        REQUIRE(scene_access.delta_count() == 0);

        { // frozen scope
            auto guard = graph_access.freeze_guard(render_thread_id);

            REQUIRE(scene_access.node_count() == 4);
            REQUIRE(scene_access.child_container_count() == 4);
            REQUIRE(scene_access.delta_count() == 0);

            // access from event thread
            REQUIRE(a->front->is_in_front());
            REQUIRE(a->back->is_in_back());

            // access from (pretend) render thread
            graph_access.set_render_thread(event_thread_id);
            CHECK(a->front->is_in_front());
            CHECK(a->back->is_in_back());
            graph_access.set_render_thread(render_thread_id);

            a->reverse();

            // access from event thread
            REQUIRE(a->front->is_in_back());
            REQUIRE(a->back->is_in_front());

            // access from (pretend) render thread
            graph_access.set_render_thread(event_thread_id);
            CHECK(a->front->is_in_front());
            CHECK(a->back->is_in_back());
            graph_access.set_render_thread(render_thread_id);

            REQUIRE(scene_access.node_count() == 4);
            REQUIRE(scene_access.child_container_count() == 4);
            REQUIRE(scene_access.delta_count() == 1);
        }

        REQUIRE(a->front->is_in_back());
        REQUIRE(a->back->is_in_front());

        REQUIRE(scene_access.node_count() == 4);
        REQUIRE(scene_access.child_container_count() == 4);
        REQUIRE(scene_access.delta_count() == 0);
    }

    SECTION("deleting nodes from a frozen scene will produce deltas that are resolved when unfrozen")
    {
        NodeHandle<TwoChildrenNode> a = scene.root().add_child<TwoChildrenNode>();
        NodeHandle<SplitNode> b = scene.root().add_child<SplitNode>();
        NodeHandle<ThreeChildrenNode> c = scene.root().add_child<ThreeChildrenNode>();

        REQUIRE(scene_access.node_count() == 12);
        REQUIRE(scene_access.child_container_count() == 12);
        REQUIRE(scene_access.delta_count() == 0);

        { // frozen scope
            auto guard = graph_access.freeze_guard(render_thread_id);

            REQUIRE(scene_access.node_count() == 12);
            REQUIRE(scene_access.child_container_count() == 12);
            REQUIRE(scene_access.delta_count() == 0);

            a->reverse();
            c->reverse();

            REQUIRE(scene_access.node_count() == 12);
            REQUIRE(scene_access.child_container_count() == 12);
            REQUIRE(scene_access.delta_count() == 2); // a and c were modified

            { // delete c
                NodeHandle<ThreeChildrenNode> dropped = std::move(c);
            }

            REQUIRE(scene_access.node_count() == 12);
            REQUIRE(scene_access.child_container_count() == 12);
            REQUIRE(scene_access.delta_count() == 3);
        }

        REQUIRE(scene_access.node_count() == 8);
        REQUIRE(scene_access.child_container_count() == 8);
        REQUIRE(scene_access.delta_count() == 0);
    }

    SECTION("nodes that are created and modified with a frozen scene will unfreeze with it")
    {
        REQUIRE(scene_access.node_count() == 1);
        REQUIRE(scene_access.child_container_count() == 1);
        REQUIRE(scene_access.delta_count() == 0);

        {
            graph_access.freeze(render_thread_id);

            NodeHandle<TwoChildrenNode> a = scene.root().add_child<TwoChildrenNode>();

            CHECK(a->front->is_in_front());
            CHECK(a->back->is_in_back());

            a->reverse();

            CHECK(a->front->is_in_back());
            CHECK(a->back->is_in_front());

            graph_access.unfreeze(render_thread_id);

            REQUIRE(a->front->is_in_back());
            REQUIRE(a->back->is_in_front());
        }

        REQUIRE(scene_access.node_count() == 1);
        REQUIRE(scene_access.child_container_count() == 1);
        REQUIRE(scene_access.delta_count() == 0);
    }
    //#else
    SECTION("nodes that are create & removed while frozen are removed correctly when unfrozen")
    {
        REQUIRE(scene_access.node_count() == 1);
        REQUIRE(scene_access.child_container_count() == 1);
        REQUIRE(scene_access.delta_count() == 0);

        { // frozen scope
            auto guard = graph_access.freeze_guard(render_thread_id);
            {
                NodeHandle<TwoChildrenNode> a = scene.root().add_child<TwoChildrenNode>();
                NodeHandle<SplitNode> b = scene.root().add_child<SplitNode>();
                NodeHandle<ThreeChildrenNode> c = scene.root().add_child<ThreeChildrenNode>();
                NodeHandle<LeafNode> d = scene.root().add_child<LeafNode>();
            }
        }

        REQUIRE(scene_access.node_count() == 1);
        REQUIRE(scene_access.child_container_count() == 1);
        REQUIRE(scene_access.delta_count() == 0);
    }
#endif
}
