#include "app/scene_property.hpp"

#include "app/scene_node.hpp"

NOTF_OPEN_NAMESPACE

//====================================================================================================================//

namespace detail {

bool is_node_frozen(valid_ptr<SceneNode*> node) { return node->graph()->is_frozen(); }

bool is_node_frozen_by(valid_ptr<SceneNode*> node, const std::thread::id& thread_id)
{
    return node->graph()->is_frozen_by(thread_id);
}

const std::string& node_name(valid_ptr<SceneNode*> node) { return node->name(); }

} // namespace detail

NOTF_CLOSE_NAMESPACE
