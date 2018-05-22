#include "catch.hpp"

#include "app/window.hpp"
#include "app/scene_node.hpp"
#include "app/scene_node_handle.hpp"
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
    SceneNodeHandle<LeafNode> back;
    SceneNodeHandle<LeafNode> front;
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
        back->stack_front();
        center->stack_back();
    }

    // fields -----------------------------------------------------------------
public:
    SceneNodeHandle<LeafNode> back;
    SceneNodeHandle<LeafNode> center;
    SceneNodeHandle<LeafNode> front;
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
    SceneNodeHandle<TwoChildrenNode> m_center;
};

//====================================================================================================================//

struct TestNode : public SceneNode {
    /// Constructor.
    TestNode(const FactoryToken& token, Scene& scene, valid_ptr<SceneNode*> parent) : SceneNode(token, scene, parent) {}

    /// Adds a new node as a child of this one.
    /// @param args Arguments that are forwarded to the constructor of the child.
    /// @throws no_graph    If the SceneGraph of the node has been deleted.
    template<class T, class... Args>
    SceneNodeHandle<T> add_child(Args&&... args)
    {
        return _add_child<T>(std::forward<Args>(args)...);
    }

    template<class T>
    void remove_child(const SceneNodeHandle<T>& handle)
    {
        _remove_child<T>(handle);
    }

    void clear() { _clear_children(); }
};

//====================================================================================================================//

SCENARIO("a Scene can be set up and modified", "[app][scene]")
{
    SceneGraphPtr scene_graph = SceneGraph::Access<test::Harness>::create(notf_window());
    std::shared_ptr<TestScene> scene_ptr = TestScene::create<TestScene>(scene_graph);
    TestScene& scene = *scene_ptr;

    SceneGraph::Access<test::Harness> graph_access(*scene_graph);
    Scene::Access<test::Harness> scene_access(scene);

    std::thread::id event_thread_id = std::this_thread::get_id();
    std::thread::id render_thread_id(1);

    SECTION("freezing an empty scene has no effect")
    {
        { // event thread
            NOTF_MUTEX_GUARD(graph_access.event_mutex());
            REQUIRE(scene_access.node_count() == 1); // root node
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

            SceneNodeHandle<TestNode> first_node = scene.root().set_child<TestNode>();         // + 1
            SceneNodeHandle<TwoChildrenNode> a = first_node->add_child<TwoChildrenNode>();     // + 3
            SceneNodeHandle<SplitNode> b = first_node->add_child<SplitNode>();                 // + 4
            SceneNodeHandle<ThreeChildrenNode> c = first_node->add_child<ThreeChildrenNode>(); // + 4 + root

            REQUIRE(scene_access.node_count() == 13);
            REQUIRE(scene_access.delta_count() == 0);

            a->reverse();
            c->reverse();

            REQUIRE(scene_access.node_count() == 13);
            REQUIRE(scene_access.delta_count() == 0);

            first_node->clear();

            REQUIRE(scene_access.node_count() == 2);
            REQUIRE(scene_access.delta_count() == 0);
        }
    }
    SECTION("modifying nodes in a frozen scene will produce deltas that are resolved when unfrozen")
    {
        SceneNodeHandle<TestNode> first_node;
        SceneNodeHandle<TwoChildrenNode> a;
        { // event thread
            NOTF_MUTEX_GUARD(graph_access.event_mutex());
            first_node = scene.root().set_child<TestNode>();
            a = first_node->add_child<TwoChildrenNode>();

            REQUIRE(scene_access.node_count() == 5);
            REQUIRE(scene_access.delta_count() == 0);
        }
        { // frozen scope
            auto guard = graph_access.freeze_guard(render_thread_id);

            { // event thread
                NOTF_MUTEX_GUARD(graph_access.event_mutex());
                REQUIRE(scene_access.node_count() == 5);
                REQUIRE(scene_access.delta_count() == 0);

                REQUIRE(a->front->is_in_front());
                REQUIRE(a->back->is_in_back());
            }
            { // render thread
                NOTF_MUTEX_GUARD(graph_access.event_mutex());
                graph_access.set_render_thread(event_thread_id);
                CHECK(a->front->is_in_front());
                CHECK(a->back->is_in_back());
                graph_access.set_render_thread(render_thread_id);
            }
            { // event thread
                NOTF_MUTEX_GUARD(graph_access.event_mutex());
                a->reverse();
                REQUIRE(a->front->is_in_back());
                REQUIRE(a->back->is_in_front());
            }
            { // render thread
                NOTF_MUTEX_GUARD(graph_access.event_mutex());
                graph_access.set_render_thread(event_thread_id);
                CHECK(a->front->is_in_front());
                CHECK(a->back->is_in_back());
                graph_access.set_render_thread(render_thread_id);

                REQUIRE(scene_access.node_count() == 5);
                REQUIRE(scene_access.delta_count() == 1);
            }
        } // end of frozen scope
        { // event thread
            NOTF_MUTEX_GUARD(graph_access.event_mutex());
            REQUIRE(a->front->is_in_back());
            REQUIRE(a->back->is_in_front());

            REQUIRE(scene_access.node_count() == 5);
            REQUIRE(scene_access.delta_count() == 0);
        }
    }

    SECTION("deleting nodes from a frozen scene will produce deltas that are resolved when unfrozen")
    {
        SceneNodeHandle<TestNode> first_node;
        SceneNodeHandle<TwoChildrenNode> a;
        SceneNodeHandle<SplitNode> b;
        SceneNodeHandle<ThreeChildrenNode> c;

        { // event thread
            NOTF_MUTEX_GUARD(graph_access.event_mutex());
            first_node = scene.root().set_child<TestNode>();
            a = first_node->add_child<TwoChildrenNode>();
            b = first_node->add_child<SplitNode>();
            c = first_node->add_child<ThreeChildrenNode>();

            REQUIRE(scene_access.node_count() == 13);
            REQUIRE(scene_access.delta_count() == 0);
        }
        { // frozen scope
            auto guard = graph_access.freeze_guard(render_thread_id);

            { // event thread
                NOTF_MUTEX_GUARD(graph_access.event_mutex());
                REQUIRE(scene_access.node_count() == 13);
                REQUIRE(scene_access.delta_count() == 0);

                a->reverse();
                c->reverse();

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
        SceneNodeHandle<TestNode> first_node;
        SceneNodeHandle<TwoChildrenNode> a;

        { // event thread
            NOTF_MUTEX_GUARD(graph_access.event_mutex());
            first_node = scene.root().set_child<TestNode>();
        }

        {
            graph_access.freeze(render_thread_id);

            { // event thread
                NOTF_MUTEX_GUARD(graph_access.event_mutex());
                a = first_node->add_child<TwoChildrenNode>();

                CHECK(a->front->is_in_front());
                CHECK(a->back->is_in_back());

                a->reverse();

                CHECK(a->front->is_in_back());
                CHECK(a->back->is_in_front());
            }

            graph_access.unfreeze(render_thread_id);

            { // event thread
                NOTF_MUTEX_GUARD(graph_access.event_mutex());
                REQUIRE(scene_access.delta_count() == 0);

                REQUIRE(a->front->is_in_back());
                REQUIRE(a->back->is_in_front());

                REQUIRE(scene_access.node_count() == 5);
            }
        }
    }

    SECTION("nodes that are create & removed while frozen do not affect the scene when unfrozen again")
    {
        SceneNodeHandle<TestNode> first_node;

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

                SceneNodeHandle<TwoChildrenNode> a = first_node->add_child<TwoChildrenNode>();
                SceneNodeHandle<SplitNode> b = first_node->add_child<SplitNode>();
                SceneNodeHandle<ThreeChildrenNode> c = first_node->add_child<ThreeChildrenNode>();
                SceneNodeHandle<LeafNode> d = first_node->add_child<LeafNode>();

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
