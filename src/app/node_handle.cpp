#include "notf/app/node_handle.hpp"

#include "notf/app/node.hpp"

NOTF_OPEN_NAMESPACE

// any node handle ================================================================================================== //

namespace detail {

void AnyNodeHandle::_remove_from_parent(NodePtr&& node) {
    if (node != nullptr) { Node::AccessFor<detail::AnyNodeHandle>::remove(node); }
}

} // namespace detail

NOTF_CLOSE_NAMESPACE
