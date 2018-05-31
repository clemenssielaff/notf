#include "app/node_property.hpp"

#include "app/node.hpp"

NOTF_OPEN_NAMESPACE

//====================================================================================================================//

NodeProperty::initial_value_error::~initial_value_error() = default;

NodeProperty::no_body_error::~no_body_error() = default;

NodeProperty::no_property_error::~no_property_error() = default;

//====================================================================================================================//

NodeProperty::~NodeProperty() = default;

bool NodeProperty::_is_frozen() const { return m_node->graph()->is_frozen(); }

bool NodeProperty::_is_frozen_by(const std::thread::id& thread_id) const
{
    return m_node->graph()->is_frozen_by(thread_id);
}

const std::string& NodeProperty::_node_name() const { return m_node->name(); }

void NodeProperty::_set_node_tweaked() const { Node::Access<NodeProperty>::register_tweaked(*m_node); }

void NodeProperty::_set_node_dirty() const { m_node->redraw(); }

NOTF_CLOSE_NAMESPACE
