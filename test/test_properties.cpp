#include "catch.hpp"

#include "app/property_batch.hpp"
#include "app/property_global.hpp"
#include "app/root_node.hpp"
#include "app/window.hpp"
#include "test_event_manager.hpp"
#include "test_node.hpp"
#include "test_properties.hpp"
#include "test_scene.hpp"
#include "test_scene_graph.hpp"
#include "testenv.hpp"

NOTF_USING_NAMESPACE

namespace {

struct SuspensionGuard {
    SuspensionGuard(EventManager& manager) : m_manager(manager) { m_manager.suspend(); }
    ~SuspensionGuard() { m_manager.resume(); }
    EventManager& m_manager;
};

} // namespace

SCENARIO("simple PropertyGraph with global properties", "[app][property_graph]")
{
    const std::thread::id event_thread_id = std::this_thread::get_id();
    const std::thread::id render_thread_id;

    auto& app = Application::instance();
    EventManager& event_manager = app.event_manager();

    SECTION("simple CRUD operations on properties")
    {
        SuspensionGuard event_suspension(event_manager);
        {
            GlobalPropertyPtr<int> iprop1 = create_global_property(48);
            GlobalPropertyPtr<int> iprop2 = create_global_property(2);
            GlobalPropertyPtr<std::string> sprop1 = create_global_property<std::string>("derbe");
            REQUIRE(PropertyGraph::Access<test::Harness>::size() == 3);

            REQUIRE(iprop1->get() == 48);
            REQUIRE(iprop2->get() == 2);
            REQUIRE(sprop1->get() == "derbe");

            iprop1->set(24);
            iprop2->set(16);
            sprop1->set("ultraderbe");

            REQUIRE(iprop1->get() == 24);
            REQUIRE(iprop2->get() == 16);
            REQUIRE(sprop1->get() == "ultraderbe");

            REQUIRE(EventManager::Access<test::Harness>(event_manager).backlog_size() == 0);
        }
        REQUIRE(PropertyGraph::Access<test::Harness>::size() == 0);
    }

    SECTION("working with property expressions")
    {
        SuspensionGuard event_suspension(event_manager);

        GlobalPropertyPtr<int> iprop1 = create_global_property<int>(48);
        GlobalPropertyPtr<int> iprop2 = create_global_property<int>(2);

        iprop1->set({}, {}); // no empty expressions
        REQUIRE(!iprop1->has_expression());
        REQUIRE(iprop1->get() == 48);

        {
            auto reader = iprop2->reader();
            iprop1->set([reader]() -> int { return reader() + 7; }, {reader});

            REQUIRE(iprop2->reader() == reader);
        }
        REQUIRE(iprop1->has_expression());

        REQUIRE(iprop1->get() == 9);
        REQUIRE(iprop2->get() == 2);

        {
            auto reader = iprop1->reader();
            REQUIRE_THROWS_AS(iprop2->set([reader]() -> int { return reader() + 7; }, {reader}),
                              PropertyGraph::no_dag_error);
            REQUIRE(iprop2->get() == 2);
        }

        iprop2.reset();
        REQUIRE(iprop1->get() == 9);
        REQUIRE(iprop1->has_expression()); // ipropr2 lives on in iprop1's expression

        iprop1->set(9);
        REQUIRE(iprop1->is_grounded());

        REQUIRE(EventManager::Access<test::Harness>(event_manager).backlog_size() == 0);
    }

    SECTION("using PropertyGraph::Batches")
    {
        {
            GlobalPropertyPtr<int> iprop1 = create_global_property<int>(48);
            GlobalPropertyPtr<int> iprop2 = create_global_property<int>(2);
            GlobalPropertyPtr<std::string> sprop1 = create_global_property<std::string>("derbe");

            PropertyBatch batch;
            {
                TypedPropertyReader<int> reader = iprop2->reader();
                batch.set(*iprop1, [reader]() -> int { return reader() + 5; }, {reader});
            }
            batch.set(*iprop2, 32);

            REQUIRE(iprop1->is_grounded());
            REQUIRE(iprop2->get() == 2);

            batch.execute();

            REQUIRE(iprop1->has_expression());
            REQUIRE(iprop1->get() == 37);
            REQUIRE(iprop2->get() == 32);
        }
        // even though iprop2 is partially owned by the expression on iprop2, it should be removed by now
        REQUIRE(PropertyGraph::Access<test::Harness>::size() == 0);
    }

    SECTION("PropertyBatches are executed automatically when they go out of scope")
    {
        GlobalPropertyPtr<int> iprop1 = create_global_property<int>(48);

        {
            PropertyBatch batch; // fails, but failure is ignored
            batch.set(*iprop1, []() -> int { throw std::runtime_error("nope"); }, {});
        }
        REQUIRE(iprop1->get() == 48);

        {
            PropertyBatch batch;
            batch.set(*iprop1, []() -> int { return 7 * 6; }, {});
        }
        REQUIRE(iprop1->get() == 42);
    }
}

SCENARIO("NodeProperties in a SceneGraph hierarchy", "[app][property_graph][scene]")
{
    SceneGraphPtr scene_graph = SceneGraph::Access<test::Harness>::create(notf_window());
    std::shared_ptr<TestScene> scene_ptr = TestScene::create<TestScene>(scene_graph, "TestScene");
    TestScene& scene = *scene_ptr;

    SceneGraph::Access<test::Harness> graph_access(*scene_graph);
    Scene::Access<test::Harness> scene_access(scene);
    using PropertyAccess = NodeProperty::Access<test::Harness>;

    const std::thread::id event_thread_id = std::this_thread::get_id();
    const std::thread::id render_thread_id;

    NodeHandle<TestNode> first_node;
    PropertyHandle<int> iprop1, iprop2;
    PropertyHandle<std::string> sprop;
    { // event thread
        NOTF_MUTEX_GUARD(graph_access.event_mutex());
        first_node = scene.get_root().set_child<TestNode>();
        iprop1 = first_node->add_property("i1", 48);
        iprop2 = first_node->add_property("i2", 0);
        sprop = first_node->add_property<std::string>("s", "before");
    }

    SECTION("change a property value in a frozen graph")
    {
        {
            auto freeze_guard = graph_access.freeze_guard(render_thread_id);

            iprop1.set(24);
            iprop2.set(0); // doesn't actually change
            sprop.set("after");

            REQUIRE(iprop1.get() == 24);
            REQUIRE(PropertyAccess::get<int>(iprop1, event_thread_id) == 24);
            REQUIRE(PropertyAccess::get<int>(iprop1, render_thread_id) == 48);

            REQUIRE(iprop2.get() == 0);
            REQUIRE(PropertyAccess::get<int>(iprop2, event_thread_id) == 0);
            REQUIRE(PropertyAccess::get<int>(iprop2, render_thread_id) == 0);

            REQUIRE(sprop.get() == "after");
            REQUIRE(PropertyAccess::get<std::string>(sprop, event_thread_id) == "after");
            REQUIRE(PropertyAccess::get<std::string>(sprop, render_thread_id) == "before");
        }

        REQUIRE(iprop1.get() == 24);
        REQUIRE(PropertyAccess::get<int>(iprop1, event_thread_id) == 24);
        REQUIRE(PropertyAccess::get<int>(iprop1, render_thread_id) == 24);

        REQUIRE(iprop2.get() == 0);
        REQUIRE(PropertyAccess::get<int>(iprop2, event_thread_id) == 0);
        REQUIRE(PropertyAccess::get<int>(iprop2, render_thread_id) == 0);

        REQUIRE(sprop.get() == "after");
        REQUIRE(PropertyAccess::get<std::string>(sprop, event_thread_id) == "after");
        REQUIRE(PropertyAccess::get<std::string>(sprop, render_thread_id) == "after");
    }

    SECTION("change a property expression in a frozen graph")
    {
        {
            auto freeze_guard = graph_access.freeze_guard(render_thread_id);

            {
                auto a = iprop1.get_reader();
                iprop2.set([=] { return a() + 1; }, {a});
            }

            REQUIRE(PropertyAccess::get(iprop2, event_thread_id) == 49);
            REQUIRE(PropertyAccess::get(iprop2, render_thread_id) == 0);
        }

        REQUIRE(PropertyAccess::get(iprop2, event_thread_id) == 49);
        REQUIRE(PropertyAccess::get(iprop2, render_thread_id) == 49);
    }

    SECTION("delete a property from a frozen graph")
    {
        auto freeze_guard = graph_access.freeze_guard(render_thread_id);

        { // event thread
            NOTF_MUTEX_GUARD(graph_access.event_mutex());
            scene.get_root().set_child<TestNode>();
        }

        REQUIRE(PropertyAccess::get<int>(iprop1, render_thread_id) == 48);
        REQUIRE(PropertyAccess::get<int>(iprop2, render_thread_id) == 0);
        REQUIRE(PropertyAccess::get<std::string>(sprop, render_thread_id) == "before");
    }

    SECTION("PropertyHandles can go out of scope")
    { // event thread
        NOTF_MUTEX_GUARD(graph_access.event_mutex());
        PropertyHandle<float> fprop = first_node->add_property<float>("f1", 48.0);
        scene.get_root().set_child<TestNode>();
        REQUIRE_THROWS_AS(fprop.set(123.f), NodeProperty::no_property_error);
    }

    SECTION("NodeProperties can have validator functions attached")
    {
        PropertyHandle<float> fprop;
        { // event thread
            NOTF_MUTEX_GUARD(graph_access.event_mutex());

            fprop = first_node->add_property("f1", 50.f, [](float& value) -> bool {
                if (value < 0) {
                    return false;
                }
                if (value > 100.f) {
                    value = 100.f;
                }
                return true;
            });
            REQUIRE(PropertyAccess::get(fprop, event_thread_id) == 50.f);

            fprop.set(99.f);
            REQUIRE(PropertyAccess::get(fprop, event_thread_id) == 99.f);

            fprop.set(101.f);
            REQUIRE(PropertyAccess::get(fprop, event_thread_id) == 100.f);

            fprop.set(-1.f);
            REQUIRE(PropertyAccess::get(fprop, event_thread_id) == 100.f);
        }
        { // frozen scope
            auto freeze_guard = graph_access.freeze_guard(render_thread_id);

            REQUIRE(PropertyAccess::get(fprop, event_thread_id) == 100.f);
            REQUIRE(PropertyAccess::get(fprop, render_thread_id) == 100.f);

            fprop.set(101.f);
            REQUIRE(PropertyAccess::get(fprop, event_thread_id) == 100.f);
            REQUIRE(PropertyAccess::get(fprop, render_thread_id) == 100.f);

            fprop.set(0.f);
            REQUIRE(PropertyAccess::get(fprop, event_thread_id) == 0.f);
            REQUIRE(PropertyAccess::get(fprop, render_thread_id) == 100.f);

            fprop.set(-1.f);
            REQUIRE(PropertyAccess::get(fprop, event_thread_id) == 0.f);
            REQUIRE(PropertyAccess::get(fprop, render_thread_id) == 100.f);
        }

        REQUIRE(PropertyAccess::get(fprop, event_thread_id) == 0.f);
        REQUIRE(PropertyAccess::get(fprop, render_thread_id) == 0.f);

        // creating a value with an invalid default throws an exception
        REQUIRE_THROWS_AS(first_node->add_property("f2", 0.f, [](float&) { return false; }),
                          NodeProperty::initial_value_error);
    }

    SECTION("NodeProperties can exist without Property bodies")
    {
        PropertyHandle<float> fprop;
        { // event thread
            NOTF_MUTEX_GUARD(graph_access.event_mutex());
            fprop = first_node->add_property("f1", 0.f, {}, /* had_body = */ false);
            REQUIRE(PropertyAccess::get(fprop, event_thread_id) == 0.f);

            fprop.set(1.f);
            REQUIRE(PropertyAccess::get(fprop, event_thread_id) == 1.f);

            fprop.set(1.f);
            REQUIRE(PropertyAccess::get(fprop, event_thread_id) == 1.f);

            REQUIRE_THROWS_AS(fprop.set([] { return 13.f + 2.f; }, {}), NodeProperty::no_body_error);
            REQUIRE(PropertyAccess::get(fprop, event_thread_id) == 1.f);
        }
        { // frozen scope
            auto freeze_guard = graph_access.freeze_guard(render_thread_id);

            REQUIRE(PropertyAccess::get(fprop, event_thread_id) == 1.f);
            REQUIRE(PropertyAccess::get(fprop, render_thread_id) == 1.f);

            fprop.set(10.f);
            REQUIRE(PropertyAccess::get(fprop, event_thread_id) == 10.f);
            REQUIRE(PropertyAccess::get(fprop, render_thread_id) == 1.f);
        }

        REQUIRE(PropertyAccess::get(fprop, event_thread_id) == 10.f);
        REQUIRE(PropertyAccess::get(fprop, render_thread_id) == 10.f);
    }
}
