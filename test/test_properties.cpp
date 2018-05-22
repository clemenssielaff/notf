#include "catch.hpp"

#include "app/window.hpp"
#include "app/property_global.hpp"
#include "test_event_manager.hpp"
#include "test_properties.hpp"
#include "testenv.hpp"

NOTF_USING_NAMESPACE

namespace {

struct SuspensionGuard {
    SuspensionGuard(EventManager& manager) : m_manager(manager) { m_manager.suspend(); }
    ~SuspensionGuard() { m_manager.resume(); }
    EventManager& m_manager;
};

} // namespace

SCENARIO("a PropertyGraph can be set up and modified", "[app][property_graph]")
{
    const std::thread::id some_thread_id{1};
    const std::thread::id other_thread_id{2};
    const std::thread::id render_thread_id{3};

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

            REQUIRE(iprop1->value() == 48);
            REQUIRE(iprop2->value() == 2);
            REQUIRE(sprop1->value() == "derbe");

            iprop1->set_value(24);
            iprop2->set_value(16);
            sprop1->set_value("ultraderbe");

            REQUIRE(iprop1->value() == 24);
            REQUIRE(iprop2->value() == 16);
            REQUIRE(sprop1->value() == "ultraderbe");

            REQUIRE(EventManager::Access<test::Harness>(event_manager).backlog_size() == 0);
        }
        REQUIRE(PropertyGraph::Access<test::Harness>::size() == 0);
    }

    SECTION("working with property expressions")
    {
        SuspensionGuard event_suspension(event_manager);

        GlobalPropertyPtr<int> iprop1 = create_global_property<int>(48);
        GlobalPropertyPtr<int> iprop2 = create_global_property<int>(2);

        {
            auto reader = iprop2->reader();
            iprop1->set_expression([reader]() -> int { return reader() + 7; }, {reader});
        }
        REQUIRE(iprop1->has_expression());

        REQUIRE(iprop1->value() == 9);
        REQUIRE(iprop2->value() == 2);

        iprop2.reset();
        REQUIRE(iprop1->value() == 9);
        REQUIRE(iprop1->has_expression()); // ipropr2 lives on in iprop1's expression

        iprop1->set_value(9);
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
                PropertyReader<int> reader = iprop2->reader();
                batch.set_expression(*iprop1, [reader]() -> int { return reader() + 5; }, {reader});
            }
            batch.set_value(*iprop2, 32);

            REQUIRE(iprop1->is_grounded());
            REQUIRE(iprop2->value() == 2);

            batch.execute();

            REQUIRE(iprop1->has_expression());
            REQUIRE(iprop1->value() == 37);
            REQUIRE(iprop2->value() == 32);
        }
        // even though iprop2 is partially owned by the expression on iprop2, it should be removed by now
        REQUIRE(PropertyGraph::Access<test::Harness>::size() == 0);
    }
}

SCENARIO("a PropertyGraph must work with a SceneGraph", "[app][property_graph][scene]")
{

#if 1
#else
    //        WHEN("create a graph with a few properties and freeze it")
    //        {
    //            std::unique_ptr<Property<int>> iprop = std::make_unique<Property<int>>(graph, 48);
    //            std::unique_ptr<Property<int>> iprop2 = std::make_unique<Property<int>>(graph, 0);
    //            std::unique_ptr<Property<std::string>> sprop = std::make_unique<Property<std::string>>(graph,
    //            "before");

    //            PropertyGraph::Access<test::Harness> graph_access(graph);

    //            graph_access.freeze(render_thread_id);

    //            AND_THEN("change a value")
    //            {
    //                iprop->set_value(24);
    //                sprop->set_value("after");

    //                THEN("the new value will replace the old one for all but the render thread")
    //                {
    //                    REQUIRE(iprop->value() == 24);
    //                    REQUIRE(graph_access.read_property(*iprop, some_thread_id) == 24);
    //                    REQUIRE(graph_access.read_property(*iprop, other_thread_id) == 24);
    //                    REQUIRE(graph_access.read_property(*iprop, render_thread_id) == 48);

    //                    REQUIRE(sprop->value() == "after");
    //                    REQUIRE(graph_access.read_property(*sprop, some_thread_id) == "after");
    //                    REQUIRE(graph_access.read_property(*sprop, other_thread_id) == "after");
    //                    REQUIRE(graph_access.read_property(*sprop, render_thread_id) == "before");
    //                }

    //                AND_WHEN("the graph is unfrozen again")
    //                {
    //                    graph_access.unfreeze();

    //                    THEN("the new value will replace the old one")
    //                    {
    //                        REQUIRE(iprop->value() == 24);
    //                        REQUIRE(graph_access.read_property(*iprop, some_thread_id) == 24);
    //                        REQUIRE(graph_access.read_property(*iprop, other_thread_id) == 24);
    //                        REQUIRE(graph_access.read_property(*iprop, render_thread_id) == 24);

    //                        REQUIRE(sprop->value() == "after");
    //                        REQUIRE(graph_access.read_property(*sprop, other_thread_id) == "after");
    //                        REQUIRE(graph_access.read_property(*sprop, some_thread_id) == "after");
    //                        REQUIRE(graph_access.read_property(*sprop, render_thread_id) == "after");
    //                    }
    //                }
    //            }

    //            AND_THEN("delete a property")
    //            {
    //                graph_access.delete_property(iprop->node().get(), some_thread_id);

    //                REQUIRE_THROWS(iprop->value()); // property is actually deleted (not possible without test
    //                harness)

    //                THEN("the render thread will still be able to access the property")
    //                {
    //                    REQUIRE(graph_access.read_property(*iprop, render_thread_id) == 48);
    //                }
    //                iprop->node() = nullptr; // to avoid double-freeing the node
    //            }

    //            AND_THEN("define an expression for a property")
    //            {
    //                Property<int>& iprop_ref = *iprop.get();
    //                iprop2->set_expression([&iprop_ref] { return iprop_ref.value() + 1; }, {iprop_ref});

    //                THEN("the expression will replace the old value for all but the render thread")
    //                {
    //                    REQUIRE(iprop2->value() == 49);
    //                    REQUIRE(graph_access.read_property(*iprop2, some_thread_id) == 49);
    //                    REQUIRE(graph_access.read_property(*iprop2, other_thread_id) == 49);
    //                    REQUIRE(graph_access.read_property(*iprop2, render_thread_id) == 0);
    //                }

    //                AND_WHEN("the graph is unfrozen again")
    //                {
    //                    graph_access.unfreeze();

    //                    THEN("the expression will replace the old value")
    //                    {
    //                        REQUIRE(iprop2->value() == 49);
    //                        REQUIRE(graph_access.read_property(*iprop2, some_thread_id) == 49);
    //                        REQUIRE(graph_access.read_property(*iprop2, other_thread_id) == 49);
    //                        REQUIRE(graph_access.read_property(*iprop2, render_thread_id) == 49);
    //                    }
    //                }
    //            }

    //            graph_access.unfreeze(); // always unfreeze!
    //        }
    //    }
#endif
}
