#include "common/mpsc_queue.hpp"

#include <iostream>
#include <string>

#include "app/core/property_graph.hpp"

using namespace notf;

namespace {} // namespace

int mpsc_main(int /*argc*/, char* /*argv*/ [])
{
    using IntProperty    = const Property<int>;
    using FloatProperty  = const Property<float>;
    using StringProperty = const Property<std::string>;

    PropertyGraph graph;
    IntProperty* int_prop       = graph.add_property<int>();
    StringProperty* string_prop = graph.add_property<std::string>("DERBNESS");
    std::cout << "Created integer property with default value: " << int_prop->value() << std::endl;
    std::cout << "Created Foo property with value: " << string_prop->value() << std::endl;

    graph.set_property(int_prop->id(), 123);
    std::cout << "Changed integer property to: " << int_prop->value() << std::endl;

    FloatProperty* a = graph.add_property(1.23456f);
    FloatProperty* b = graph.add_property(2.5f);
    FloatProperty* c = graph.add_property(0.f);
    graph.set_expression<float>(c->id(), [a, b]() { return a->value() + b->value(); }, {a->id(), b->id()});

    std::cout << "Evaluating expression for c: " << c->value() << std::endl;

    graph.set_property(a->id(), 2.5678f);
    std::cout << "Evaluating expression after changing a: " << c->value() << std::endl;

    graph.set_property(c->id(), 4.6f);
    std::cout << "Explicitly set value of c: " << c->value() << std::endl;

    graph.set_expression<float>(c->id(), [a, b]() { return a->value() + b->value(); }, {a->id(), b->id()});
    std::cout << "Evaluating expression for c (again): " << c->value() << std::endl;

    bool better_not
        = graph.set_expression<float>(a->id(), [b, c]() { return b->value() + c->value(); }, {b->id(), c->id()});
    if (better_not) {
        std::cout << "Created circular dependency! OH NOOOOOoooo" << std::endl;
    }
    else {
        std::cout << "Detected circular dependency! ... nice :)" << std::endl;
    }

    graph.delete_property(a->id());
    graph.set_property(b->id(), 0.0f);
    std::cout << "Evaluating expression for c afer removing a: " << c->value() << std::endl;

    return 0;
}
