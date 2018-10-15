#include <iostream>

#include "notf/app/property.hpp"

NOTF_USING_NAMESPACE;

template<class T>
struct AProperty : Property<T> {

    AProperty(T value) : Property<T>(value) {}

    /// The Node-unique name of this Property.
    std::string_view get_name() const override { return {}; }
    void _clear_frozen() override {}
};

int main()
{

    auto blub = AProperty(45);

    return 0; //
}
