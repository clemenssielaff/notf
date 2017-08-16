#if 0

#define NOTF_NO_SIMD

#include "utils/debug.hpp"
#include <common/vector4.hpp>
#include <common/xform3.hpp>
#include <iostream>

using namespace notf;

/** MAIN
 */
int main(int argc, char* argv[])
{
    Xform3f pm = Xform3f::identity();
    pm[0][0]   = 2.0f / 800.0f; // 1.0f;
    pm[0][1]   = 0.0f;
    pm[0][2]   = 0.0f;
    pm[0][3]   = 0.0f;

    pm[1][0] = 0.0f;
    pm[1][1] = 2.0f / 600.f; // 1.0f;
    pm[1][2] = 0.0f;
    pm[1][3] = 0.0f;

    pm[2][0] = 0.0f;
    pm[2][1] = 0.0f;
    pm[2][2] = 1.0f; // 1.0f;
    pm[2][3] = 0.0f;

    pm[3][0] = -1.0; // 0.0f;
    pm[3][1] = -1.0; // 0.0f;
    pm[3][2] = 0.0f;
    pm[3][3] = 1.0f;

    //    const Vector4f pos(800, 600, 0, 1);
    const Vector4f pos(0, 0, 0, 1);

    Vector4f result = pm.transform(pos);

    std::cout << result << std::endl;

    return 0;
}

#endif
