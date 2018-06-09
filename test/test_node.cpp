#include "catch.hpp"

#include "test_node.hpp"

NOTF_USING_NAMESPACE

// ================================================================================================================== //

TestNode::~TestNode() = default;

void TestNode::reverse_children()
{
    NOTF_MUTEX_GUARD(_hierarchy_mutex());
    _write_children().reverse();
}
