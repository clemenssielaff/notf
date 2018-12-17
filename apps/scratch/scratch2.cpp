#include <iostream>

#include "notf/meta/stringtype.hpp"
#include "notf/meta/tuple.hpp"
NOTF_USING_NAMESPACE;

struct Int {
    constexpr static int value = 5;
    using type = int;
    static constexpr ConstString name = "Int";
    static constexpr const ConstString& get_const_name() noexcept { return name; }
};
struct Float {
    constexpr static int value = 6;
    using type = float;
    static constexpr ConstString name = "Float";
    static constexpr const ConstString& get_const_name() noexcept { return name; }
};
struct Bool {
    constexpr static int value = 7;
    using type = bool;
    static constexpr ConstString name = "Bool";
    static constexpr const ConstString& get_const_name() noexcept { return name; }
};

using Tuple = std::tuple<std::shared_ptr<Int>, std::shared_ptr<Float>, std::shared_ptr<Bool>>;

int main() {
    Tuple tuple;

    visit_at(tuple, 1, [](const auto& value) { std::cout << std::decay_t<decltype(value)>::element_type::name.c_str() << std::endl; });
    //    std::cout << result << std::endl;
    //        = find_in_tuple<0>(Tuple{}, [](const auto& type) { return std::decay_t<decltype(type)>::value == 6; });

    return 0;
}
