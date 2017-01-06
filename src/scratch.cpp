#include <iostream>
#include <string>

#include "core/property.hpp"
#include "core/property_impl.hpp"

using namespace notf;

template <typename TARGET_TYPE>
constexpr void connect_property_signals(Property<TARGET_TYPE>*)
{
    // stop recursion
}

template <typename TARGET_TYPE, typename T, typename... ARGS,
          typename = std::enable_if_t<!std::is_base_of<AbstractProperty, std::remove_pointer_t<std::decay_t<T>>>::value>>
constexpr void connect_property_signals(Property<TARGET_TYPE>* target, T&&, ARGS&&... args)
{
    // ignore non-Property `T`s
    return connect_property_signals(target, std::forward<ARGS>(args)...);
}

template <typename TARGET_TYPE, typename SOURCE_TYPE, typename... ARGS>
constexpr void connect_property_signals(Property<TARGET_TYPE>* target, Property<SOURCE_TYPE>* source, ARGS&&... args)
{
    // connect the value_changed signal of all Property arguments
    target->connect_signal(source->value_changed, [target](SOURCE_TYPE) { target->_update_expression(); });
    return connect_property_signals(target, std::forward<ARGS>(args)...);
}

//int notmain()
int main()
{
    PropertyMap map;
    IntProperty* one = reinterpret_cast<IntProperty*>(add_property(map, "one", 1)->second.get());
    IntProperty* two = reinterpret_cast<IntProperty*>(add_property(map, "two", 2)->second.get());

    std::cout << one->get_value() << std::endl;
    std::cout << two->get_value() << std::endl;

    one->_set_expression([two] { return two->get_value(); });
    connect_property_signals(one, two);

    std::cout << "--------------" << std::endl;
    std::cout << one->get_value() << std::endl;
    std::cout << two->get_value() << std::endl;

    two->set_value(12);

    std::cout << "--------------" << std::endl;
    std::cout << one->get_value() << std::endl;
    std::cout << two->get_value() << std::endl;

    return 0;
}
