#include "notf/app/node_handle.hpp"

#include "notf/app/node.hpp"

NOTF_OPEN_NAMESPACE

// any node handle ================================================================================================== //

namespace detail {

Uuid AnyNodeHandle::_get_uuid(NodePtr&& node) { return node->get_uuid(); }

std::string AnyNodeHandle::_get_name(NodePtr&& node) { return node->get_name(); }

void AnyNodeHandle::_set_name(NodePtr&& node, const std::string& name)
{
    NOTF_GUARD(std::lock_guard(TheGraph::get_graph_mutex()));
    node->set_name(name);
}

void AnyNodeHandle::_remove_from_parent(NodePtr&& node)
{
    if (node != nullptr) { Node::AccessFor<detail::AnyNodeHandle>::remove(node); }
}

} // namespace detail

NOTF_CLOSE_NAMESPACE
