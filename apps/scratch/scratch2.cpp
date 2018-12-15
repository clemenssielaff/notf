#include <iostream>
#include <memory>

#include "notf/common/bimap.hpp"
NOTF_USING_NAMESPACE;

int main() {

    Bimap<int, std::string> derbemap;

    derbemap.set(23, std::string("derbe"));
    derbemap.set(std::string("veryderbe"), 24);

    std::cout << derbemap.get(23) << std::endl;
    std::cout << derbemap.get(24) << std::endl;
    std::cout << derbemap.get("derbe") << std::endl;
    std::cout << derbemap.get("underbe") << std::endl;

    return 0;
}
