#include <iostream>

#include "notf/common/rational.hpp"
//#include "notf/common/random.hpp"

NOTF_USING_NAMESPACE;

int main()
{
    Ratioi derbe(64, 2);

    std::cout << derbe + 4 << std::endl;
    std::cout << 4 + derbe << std::endl;

    return 0;
}
