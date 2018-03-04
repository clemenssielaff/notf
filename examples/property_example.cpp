#include "common/mpsc_queue.hpp"

#include <iostream>
#include <string>

#include "app/core/property_graph.hpp"
#include "app/core/property_manager.hpp"
#include "common/log.hpp"

NOTF_USING_NAMESPACE

namespace {

void test_property_graph()
{
    PropertyGraph graph;

    PropertyId int_prop = graph.next_id();
    graph.add_property<int>(int_prop);

    PropertyId string_prop = graph.next_id();
    graph.add_property<std::string>(string_prop);

    graph.set_property<std::string>(string_prop, "Derbness");
    std::cout << "Created integer property with default value: " << graph.property<int>(int_prop) << std::endl;
    std::cout << "Created Foo property with value: " << graph.property<std::string>(string_prop) << std::endl;

    graph.set_property(int_prop, 123);
    std::cout << "Changed integer property to: " << graph.property<int>(int_prop) << std::endl;

    PropertyId a = graph.next_id();
    graph.add_property<float>(a);
    PropertyId b = graph.next_id();
    graph.add_property<float>(b);
    PropertyId c = graph.next_id();
    graph.add_property<float>(c);

    graph.set_property<float>(a, 1.23456f);
    graph.set_property<float>(b, 2.5f);
    graph.set_expression<float>(
        c, [a, b](const PropertyGraph& graph) -> float { return graph.property<float>(a) + graph.property<float>(b); },
        {a, b});

    std::cout << "Evaluating expression for c (expecting 3.73456): " << graph.property<float>(c) << std::endl;

    graph.set_property<float>(a, 2.5678f);
    std::cout << "Evaluating expression after changing a (expecting 5.0678): " << graph.property<float>(c) << std::endl;

    graph.set_property<float>(c, 4.6f);
    std::cout << "Explicitly set value of c to 4.6 (expecting 4.6): " << graph.property<float>(c) << std::endl;

    graph.set_expression<float>(
        c, [a, b](const PropertyGraph& graph) -> float { return graph.property<float>(a) + graph.property<float>(b); },
        {a, b});
    std::cout << "Evaluating expression for c (again) (expecting 5.0678): " << graph.property<float>(c) << std::endl;

    bool caught = false;
    try {
        graph.set_expression<float>(
            a, [b, c](const PropertyGraph& graph) { return graph.property<float>(b) + graph.property<float>(c); },
            {b, c});
    }
    catch (...) {
        caught = true;
    }
    if (caught) {
        std::cout << "Detected circular dependency! ... nice :)" << std::endl;
    }
    else {
        std::cout << "Created circular dependency! OH NOOOOOoooo" << std::endl;
    }

    graph.delete_property(a);
    graph.set_property<float>(b, 0.0f);
    std::cout << "Evaluating expression for c afer removing a (expecting 5.0678): " << graph.property<float>(c)
              << std::endl;
}

struct Arsch {};

void test_property_manager()
{
    PropertyManager manager;
    const PropertyGraph& graph = manager.graph();

    PropertyManager::CommandBatch batch1 = manager.create_batch(Time{});

    auto a = batch1.create_property<float>();
    batch1.set_property(a, 0.4);

    auto b = batch1.create_property(0.7f);

    auto c = batch1.create_property<float>(0);
    batch1.set_expression(c, [a, b](const PropertyGraph& g) { return g.property(a) + g.property(b); }, {a, b});

    //    batch.delete_property(a);

    manager.schedule_batch(std::move(batch1));
    manager.execute_batches();

    std::cout << "Value of a (expecting 0.4): " << graph.property(a) << std::endl;
    std::cout << "Value of b (expecting 0.7): " << graph.property(b) << std::endl;
    std::cout << "Value of c (expecting 1.1): " << graph.property(c) << std::endl;

    PropertyManager::CommandBatch batch2 = manager.create_batch(Time{});

    batch1.set_expression(a, [b, c](const PropertyGraph& g) { return g.property(b) + g.property(c); }, {b, c});

    manager.schedule_batch(std::move(batch1));
    manager.execute_batches();

    std::cout << "Value of a (expecting 0.4): " << graph.property(a) << std::endl;
    std::cout << "Value of b (expecting 0.7): " << graph.property(b) << std::endl;
    std::cout << "Value of c (expecting 1.1): " << graph.property(c) << std::endl;
}

} // namespace

int property_main(int /*argc*/, char* /*argv*/ [])
{
    auto log_handler = std::make_unique<LogHandler>(128, 200);
    install_log_message_handler(std::bind(&LogHandler::push_log, log_handler.get(), std::placeholders::_1));
    log_handler->start();

    //    test_property_graph();
    test_property_manager();

    log_handler->stop();
    log_handler->join();
    return 0;
}
