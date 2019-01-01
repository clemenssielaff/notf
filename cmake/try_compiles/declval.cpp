/// GCC fails to compile `decltype(declval<T>())`
#include <utility>

struct AType {
    inline static constexpr int value = 0;
};
struct BType {
    inline static constexpr int value = 1;
};

template<unsigned I, class A, class B>
constexpr auto produce_type() {
    if constexpr (I == 0) {
        return std::declval<A>();
    } else {
        return std::declval<B>();
    }
}

int main() {
    using foo = decltype(produce_type<0, AType, BType>());
//    static_assert(decltype(produce_type<0, AType, BType>())::value == 0);
    return 0;
}

