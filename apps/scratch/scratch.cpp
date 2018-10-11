#include <iostream>
#include <sstream>

#include "notf/common/uuid.hpp"

NOTF_USING_NAMESPACE;

int main()
{
//    for (auto i = 0; i < 1; ++i) {
        const auto uuid = Uuid::generate();
        std::cout << uuid;
//    }
    return 0;
}
