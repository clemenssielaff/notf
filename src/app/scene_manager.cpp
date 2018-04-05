#include "app/scene_manager.hpp"

#include "app/scene_node.hpp"
#include "app/window.hpp"

NOTF_OPEN_NAMESPACE

//====================================================================================================================//

namespace detail {

SceneNodeBase::~SceneNodeBase() = default;

} // namespace detail

//====================================================================================================================//

SceneManager::DeletionDelta::~DeletionDelta() = default;

//====================================================================================================================//

SceneManager::SceneManager(Window& window) : m_window(window), m_current_state(create_state({})) {}

SceneManager::~SceneManager() = default;

void SceneManager::enter_state(StatePtr state)
{
    m_current_state = std::move(state);
    m_window.request_redraw();
}

void SceneManager::freeze()
{
    const auto thread_id = std::this_thread::get_id();
    {
        std::lock_guard<Mutex> lock(m_mutex);
        if (thread_id == m_render_thread) {
            return;
        }
        if (thread_id != std::thread::id(0)) {
            notf_throw(thread_error, "Unexpected reading thread of a PropertyGraph");
        }
        m_render_thread = thread_id;
    }
}

void SceneManager::unfreeze()
{
    const auto thread_id = std::this_thread::get_id();
    {
        std::lock_guard<Mutex> lock(m_mutex);
        if (thread_id != m_render_thread) {
            notf_throw(thread_error, "Only the render thread can unfreeze the PropertyGraph");
        }

        for (const auto& delta_it : m_delta) {
            auto node_it = m_nodes.find(delta_it.first);
            NOTF_ASSERT(node_it != m_nodes.end());
            valid_ptr<SceneNodeBase*> delta = delta_it.second.get();

            if (is_deletion_delta(delta)) {
                m_nodes.erase(node_it);
            }
            else {
                // modification delta
                // TODO: this ain't right - how do we unfreeze SceneNodes?
            }
        }
        m_delta.clear();
    }
}

//====================================================================================================================//

valid_ptr<SceneNode*> SceneNodeParent::get_mutable()
{
    SceneManager& scene = m_node->manager();
    {
        std::lock_guard<Mutex> lock(scene.m_mutex);
        return static_cast<SceneNode*>(scene.write_node(m_node).get());
    }
}

valid_ptr<const SceneNode*> SceneNodeParent::get_const() const
{
    SceneManager& scene = m_node->manager();
    {
        std::lock_guard<Mutex> lock(scene.m_mutex);
        return static_cast<const SceneNode*>(scene.read_node(m_node, std::this_thread::get_id()).get());
    }
}

//====================================================================================================================//

namespace detail {

valid_ptr<SceneNode*> SceneNodeChildHelper::get_mutable(valid_ptr<SceneNodeBase*> node)
{
    SceneManager& scene = node->manager();
    {
        std::lock_guard<Mutex> lock(scene.m_mutex);
        return static_cast<SceneNode*>(scene.write_node(node).get());
    }
}

valid_ptr<const SceneNode*> SceneNodeChildHelper::get_const(valid_ptr<SceneNodeBase*> node)
{
    SceneManager& scene = node->manager();
    {
        std::lock_guard<Mutex> lock(scene.m_mutex);
        return static_cast<const SceneNode*>(scene.read_node(node, std::this_thread::get_id()).get());
    }
}

} // namespace detail

NOTF_CLOSE_NAMESPACE
