#include "notf/app/graph/root_node.hpp"

#include "notf/app/graph/window.hpp"

NOTF_OPEN_NAMESPACE

// root node ======================================================================================================== //

void RootNode::_add_window(WindowPtr window) {
    AnyNodePtr node = std::static_pointer_cast<AnyNode>(std::move(window));
    std::vector<AnyNodePtr>& children = AnyNode::AccessFor<RootNode>::write_children(*this);
    NOTF_ASSERT(std::find(children.begin(), children.end(), window) == children.end());
    children.emplace_back(std::move(window));
}

void RootNode::_remove_window(const Window* window) {
    auto node = static_cast<const AnyNode*>(window);
    std::vector<AnyNodePtr>& children = AnyNode::AccessFor<RootNode>::write_children(*this);
    if (auto itr = std::find_if(children.begin(), children.end(), //
                                [node](const auto& child) { return child.get() == node; });
        itr != children.end()) {
        children.erase(itr);
    }
}

NOTF_CLOSE_NAMESPACE
