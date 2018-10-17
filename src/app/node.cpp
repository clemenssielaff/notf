#include <utility>

#include "notf/app/node.hpp"

NOTF_OPEN_NAMESPACE

// node ============================================================================================================= //

Node::Node(valid_ptr<Node*> parent) : m_parent(parent)
{

    // register the Node with the Graph
#ifdef NOTF_DEBUG
    try {
        TheGraph::AccessFor<Node>::register_node(NodeHandle(shared_from_this()));
    }
    catch (const std::bad_weak_ptr&) {
        NOTF_ASSERT(false, "Nodes must always be managed by a shared_ptr");
    }
#else
    TheGraph::Private<Node>::register_node(NodeHandle(weak_from_this()));
#endif
}

Node::~Node() { TheGraph::AccessFor<Node>::unregister_node(m_uuid); }

NOTF_CLOSE_NAMESPACE
