#if 1

//#define NOTF_NO_SIMD

#include <common/vector4.hpp>
#include <iostream>

using namespace notf;
using V4f = notf::Vector4f;

/** MAIN
 */
int main(int argc, char* argv[])
{
    V4f a(1, 0, 0);
    V4f b(0, 1, 0);

    V4f result = a.get_crossed(b);

    std::cout << result << std::endl;

    return 0;
}

#endif
