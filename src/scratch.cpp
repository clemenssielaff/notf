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
    V4f a = V4f::fill(4);
    V4f b = V4f::fill(1);
    V4f c = {238, 1.2, 0.3f};
    V4f d = b;
    V4f e(0.1, 1, 0);

    //    _mm_store_ps(&a[0], _mm_add_ps(_mm_load_ps(&a.array[0]), _mm_load_ps(&b.array[0])));

    //    V4f result;
    auto result = a + b;

    //    _mm_store_ps(&result[0], _mm_add_ps(_mm_load_ps(&a.array[0]), _mm_load_ps(&b.array[0])));

    //     _mm_cvtss_f32(mmSum);

    //    for (size_t i = 0; i < result.size(); ++i) {
    std::cout << result << std::endl;
    //    }

    //    std::cout << std::hash<V4f>{}(result) << std::endl;

    return 0;
}

#endif
