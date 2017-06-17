#if 0

#include <common/arithmetic.hpp>
#include <iostream>

using namespace notf;

namespace notf {
namespace detail {

/** 3-dimensional value with elements accessible through `x`, `y` and `z` fields. */
template <typename VALUE_TYPE>
struct Value3 : public Value<VALUE_TYPE, 3> {

    Value3() = default;

    template <typename A, typename B, typename C>
    Value3(A x, B y, C z)
        : array{{static_cast<VALUE_TYPE>(x), static_cast<VALUE_TYPE>(y), static_cast<VALUE_TYPE>(z)}} {}

    union {
        std::array<VALUE_TYPE, 3> array;
        struct {
            VALUE_TYPE x;
            VALUE_TYPE y;
            VALUE_TYPE z;
        };
    };
};

} // namespace detail

} // namespace notf

struct V3f : public detail::Arithmetic<V3f, detail::Value3<float>> {
    using super_t = detail::Arithmetic<V3f, detail::Value3<float>>;
    using super_t::x;
    using super_t::y;
    using super_t::z;

    V3f() = default;

    template <typename... T>
    V3f(T&&... ts)
        : super_t{std::forward<T>(ts)...} {}

    bool is_approx(const V3f& other, const value_t epsilon = precision_high<value_t>()) const
    {
        return true;
    }
};

namespace std {
template <>
struct hash<V3f> {
    size_t operator()(const V3f& x) const { return x.hash(); }
};
}

static_assert(std::is_pod<V3f>::value, "This compiler does not recognize notf::V3f as a POD.");

/** MAIN
 */
int main(int argc, char* argv[])
{
    V3f from{1.f, 1, 1.};
    V3f to     = V3f::fill(3);
    V3f result = lerp(from, to, 0.5);
    V3f test(2, 2., 2.f);

    for (size_t i = 0; i < result.size(); ++i) {
        std::cout << result[i] << std::endl;
        assert(result[i] == test[i]);
    }

    //std::cout << std::hash<detail::Arithmetic<V3f, detail::Value3<float>>>{}(test) << std::endl;
    std::cout << std::hash<V3f>{}(test) << std::endl;

    return 0;
}

#endif
