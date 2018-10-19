#include <iostream>

#include "notf/app/node_compiletime.hpp"

NOTF_USING_NAMESPACE;

struct PositionProp {
    using value_t = float;
    static constexpr StringConst name = "position";
    static constexpr value_t default_value = 0.123f;
    static constexpr bool is_visible = true;
};
struct VisibilityProp {
    using value_t = bool;
    static constexpr StringConst name = "visible";
    static constexpr value_t default_value = true;
    static constexpr bool is_visible = true;
};

struct NodePolicy {
    using properties = std::tuple<            //
        CompileTimeProperty<PositionProp>,    //
        CompileTimeProperty<VisibilityProp>>; //
};

using MyNode = CompileTimeNode<NodePolicy>;

int main()
{
    NOTF_USING_LITERALS;
    auto manode = std::make_shared<MyNode>();
    std::cout << manode->get_property<float>("position").get() << std::endl;
    std::cout << manode->get_property("position"_id).get() << std::endl;
    return 0;
}
