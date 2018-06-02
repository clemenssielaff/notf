#include "app/node_handle.hpp"

#include "app/node.hpp"

NOTF_OPEN_NAMESPACE

// ================================================================================================================== //

detail::NodeHandleBase::no_node_error::~no_node_error() = default;

const std::string& detail::NodeHandleBase::_name(valid_ptr<const Node*> node) { return node->name(); }

NOTF_CLOSE_NAMESPACE
