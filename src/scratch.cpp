#if 1

#define NOTF_NO_SIMD

#include <common/vector4.hpp>
#include <iostream>

#include <emmintrin.h>

using namespace notf;
using V4f = notf::Vector4f;

/** MAIN
 */
int main(int argc, char* argv[])
{
    V4f a = V4f::fill(4);
    V4f b = V4f::fill(1);

    //    _mm_store_ps(&a[0], _mm_add_ps(_mm_load_ps(&a.array[0]), _mm_load_ps(&b.array[0])));

    //    V4f result;
    V4f result = a + b;

    //    _mm_store_ps(&result[0], _mm_add_ps(_mm_load_ps(&a.array[0]), _mm_load_ps(&b.array[0])));

    //     _mm_cvtss_f32(mmSum);

    //    for (size_t i = 0; i < result.size(); ++i) {
    std::cout << result << std::endl;
    //    }

    //    std::cout << std::hash<V4f>{}(result) << std::endl;

    return 0;
}

#endif
