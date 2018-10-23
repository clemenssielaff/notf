#include <iostream>
#include <memory>
#include <string>
#include <tuple>
#include <vector>

#include "notf/app/root_node.hpp"

NOTF_USING_NAMESPACE;

struct Movable {
    NOTF_NO_COPY_OR_ASSIGN(Movable);
    Movable(std::shared_ptr<int> j) : i(j) {}
    Movable() = default;
    Movable& operator=(Movable&& other) = default;

    std::weak_ptr<int> i;
};

int main()
{
    std::shared_ptr<int> shr = std::make_shared<int>(45);
    auto m = Movable(shr);

    std::cout << (m.i.expired()) << std::endl;

    Movable j;
    j = std::move(m);
    std::cout << (m.i.expired()) << std::endl;

    return 0;
}
