#include <iostream>

#include "notf/common/random.hpp"
#include "notf/common/vector3.hpp"
#include "notf/common/vector4.hpp"

NOTF_USING_NAMESPACE;
NOTF_USING_LITERALS;

int main()
{

    V3d blub1 = random<V3f>();

    std::cout << "Random V3f: " << blub1 << std::endl;
    std::cout << "Random string: " << random<std::string>(32, "abcdefghijklmnopqrstuvwxyz ") << std::endl;
}
