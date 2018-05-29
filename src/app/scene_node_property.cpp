#include "app/scene_node_property.hpp"

#include "app/scene_node.hpp"

NOTF_OPEN_NAMESPACE

//====================================================================================================================//

SceneNodeProperty::initial_value_error::~initial_value_error() = default;

SceneNodeProperty::no_body_error::~no_body_error() = default;

SceneNodeProperty::no_property_error::~no_property_error() = default;

//====================================================================================================================//

SceneNodeProperty::~SceneNodeProperty() = default;

bool SceneNodeProperty::_is_frozen() const { return m_node->graph()->is_frozen(); }

bool SceneNodeProperty::_is_frozen_by(const std::thread::id& thread_id) const
{
    return m_node->graph()->is_frozen_by(thread_id);
}

const std::string& SceneNodeProperty::_node_name() const { return m_node->name(); }

void SceneNodeProperty::_set_node_tweaked() const { SceneNode::Access<SceneNodeProperty>::register_tweaked(*m_node); }

void SceneNodeProperty::_set_node_dirty() const { m_node->redraw(); }

NOTF_CLOSE_NAMESPACE
