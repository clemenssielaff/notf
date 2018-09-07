#include <iostream>
using namespace std;

#include "notf/meta/meta.hpp"

int main()
{
    std::cout << NOTF_BUILT_COMMIT << (NOTF_BUILT_COMMIT_WAS_MODIFIED ? " (modified)" : "") << std::endl;
    return 0;
}

