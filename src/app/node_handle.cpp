#include "notf/app/node_handle.hpp"

#include "notf/app/node.hpp"

NOTF_OPEN_NAMESPACE

// node handle base ================================================================================================= //

namespace detail {

const Uuid& NodeHandleBase::get_uuid() const { return _get_node()->get_uuid(); }

std::string_view NodeHandleBase::get_name() const { return _get_node()->get_name(); }

} // namespace detail

// node master handle =============================================================================================== //

NodeMasterHandle::NodeMasterHandle(NodePtr&& node) : detail::NodeHandleBase(node)
{
    if (node == nullptr) { NOTF_THROW(ValueError, "Cannot construct a NodeMasterHandle with an empty pointer"); }
}

NodeMasterHandle::~NodeMasterHandle()
{
    if (auto node = m_node.lock()) { Node::AccessFor<NodeMasterHandle>::remove(node); }
}

// node master handle castable ====================================================================================== //

namespace detail {

NodeMasterHandleCastable::operator NodeMasterHandle()
{
    if (auto node = m_node.lock()) {
        m_node.reset();
        return NodeMasterHandle(std::move(node));
    }
    NOTF_THROW(ValueError, "Cannot create a NodeMasterHandle for a Node that is already expired");
}

} // namespace detail

NOTF_CLOSE_NAMESPACE
