#if 0

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
    Xform4f rot = Xform4f::fill(8);

    auto result = rot / 4;

//    {
//        DebugTimer __timer("2000000000 transformations:");
//        for (volatile size_t i = 0; i < 2000000000; ++i) {
//            Vector4f result = rot.transform(x);
//        }
//    }

        std::cout << result[0] << std::endl;

    return 0;
}

#endif
