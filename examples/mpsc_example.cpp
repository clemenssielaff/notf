#include "common/mpsc_queue.hpp"

#include <iostream>
#include <string>

#include "app/core/property_graph.hpp"

using namespace notf;

namespace {

} // namespace

int mpsc_main(int /*argc*/, char* /*argv*/ [])
{
    PropertyGraph graph;
    PropertyGraph::Id int_prop = graph.add_property<int>();
    PropertyGraph::Id foo_prop = graph.add_property<std::string>();
    std::cout << "Created integer property with default value: " << *graph.property<int>(int_prop) << std::endl;
    std::cout << "Created Foo property with default value: " << *graph.property<std::string>(foo_prop) << std::endl;

    graph.set_property<int>(int_prop, 123);
    std::cout << "Changed integer property to: " << *graph.property<int>(int_prop) << std::endl;

    return 0;
}
