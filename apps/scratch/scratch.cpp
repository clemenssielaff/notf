#include <iostream>

#include "notf/meta/assert.hpp"

bool funcy()
{
    std::cout << "Funkey!" << std::endl;
    return true;
}

int main()
{
    NOTF_ASSERT(funcy());
    NOTF_ASSERT_ALWAYS(funcy());

    return 0;
}
