#include <iostream>
#include <string>

#include "core/property.hpp"
#include "core/property_impl.hpp"

using namespace notf;

struct Bar {
    Bar()
        : map()
    {
        map.create_property<IntProperty>("bar_prop", 123);
    }
    PropertyMap map;
};

struct Foo {

    Foo(Bar& bar)
        : m_map()
    {
        auto prop = m_map.create_property<IntProperty>("foo_prop", 0);
        auto other = bar.map.get<IntProperty>("bar_prop");
        property_expression(prop, {return other->get_value(); }, other);
    }

    PropertyMap m_map;
};

//int notmain()
int main()
{
    PropertyMap map;

    FloatProperty* one = map.create_property<FloatProperty>("one", 1.2);
    IntProperty* two = map.create_property<IntProperty>("two", 2);
    IntProperty* three = map.create_property<IntProperty>("three", 3);
    int four = 4;

    std::cout << one->get_value() << std::endl;
    std::cout << two->get_value() << std::endl;
    std::cout << three->get_value() << std::endl;

    property_expression(one, {
        return two->get_value() + three->get_value() + four;
    },
                        two, three, four);

    std::cout << "--------------" << std::endl;
    std::cout << one->get_value() << std::endl;
    std::cout << two->get_value() << std::endl;
    std::cout << three->get_value() << std::endl;

    two->set_value(12);

    std::cout << "--------------" << std::endl;
    std::cout << one->get_value() << std::endl;
    std::cout << two->get_value() << std::endl;
    std::cout << three->get_value() << std::endl;

    Bar bar;
    Foo foo(bar);
    std::cout << "--------------" << std::endl;
    std::cout << foo.m_map.get<IntProperty>("foo_prop")->get_value() << std::endl;

    return 0;
}
