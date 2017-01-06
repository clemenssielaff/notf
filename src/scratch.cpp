#include <iostream>
#include <string>

#include "core/property.hpp"
#include "core/property_impl.hpp"

using namespace notf;

//int notmain()
int main()
{
    PropertyMap map;
    IntProperty* one = add_property<IntProperty>(map, "one", 1);
    IntProperty* two = add_property<IntProperty>(map, "two", 2);
    IntProperty* three = add_property<IntProperty>(map, "three", 3);
    int four = 4;

    std::cout << one->get_value() << std::endl;
    std::cout << two->get_value() << std::endl;
    std::cout << three->get_value() << std::endl;

    property_expression(one, {
        return two->get_value() + three->get_value() + four;
    }, two, three, four);

    std::cout << "--------------" << std::endl;
    std::cout << one->get_value() << std::endl;
    std::cout << two->get_value() << std::endl;
    std::cout << three->get_value() << std::endl;

    two->set_value(12);

    std::cout << "--------------" << std::endl;
    std::cout << one->get_value() << std::endl;
    std::cout << two->get_value() << std::endl;
    std::cout << three->get_value() << std::endl;

    return 0;
}
