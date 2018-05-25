#include "app/scene_property.hpp"

#include "app/scene_node.hpp"

NOTF_OPEN_NAMESPACE

//====================================================================================================================//

SceneProperty::initial_value_error::~initial_value_error() = default;

SceneProperty::no_body_error::~no_body_error() = default;

//====================================================================================================================//

SceneProperty::~SceneProperty() = default;

bool SceneProperty::_is_frozen() const { return m_node->graph()->is_frozen(); }

bool SceneProperty::_is_frozen_by(const std::thread::id& thread_id) const
{
    return m_node->graph()->is_frozen_by(thread_id);
}

const std::string& SceneProperty::_node_name() const { return m_node->name(); }

void SceneProperty::_set_node_tweaked() const { SceneNode::Access<SceneProperty>::register_tweaked(*m_node); }

void SceneProperty::_set_node_dirty() const { m_node->redraw(); }

NOTF_CLOSE_NAMESPACE
