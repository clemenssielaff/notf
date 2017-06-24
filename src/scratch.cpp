#if 1

#define NOTF_NO_SIMD

#include "utils/debug.hpp"
#include <common/vector4.hpp>
#include <common/xform4.hpp>
#include <iostream>

using namespace notf;

/** MAIN
 */
int main(int argc, char* argv[])
{
    Vector4f x(1, 0, 0);
    Xform4f rot = Xform4f::rotation(pi<float>() / 2.f, Vector4f{0, 1, 0, 0});

    {
        DebugTimer __timer("2000000000 transformations:");
        for (volatile size_t i = 0; i < 2000000000; ++i) {
            Vector4f result = rot.transform(x);
        }
    }

    //    std::cout << result << std::endl;

    return 0;
}

#endif
