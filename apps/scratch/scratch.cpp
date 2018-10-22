#include <iostream>
#include <memory>
#include <string>
#include <tuple>
#include <vector>

#include "notf/app/root_node.hpp"

NOTF_USING_NAMESPACE;

int main() {

    NOTF_GUARD(std::lock_guard(TheGraph::get_graph_mutex()));
    auto rn =std::make_shared<RootNode>();
    rn->make_foo();
    return 0;
}
