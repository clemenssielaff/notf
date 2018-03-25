#include "app/scene_node.hpp"

#include <atomic>
#include <unordered_set>

#include "app/application.hpp"
#include "common/log.hpp"
#include "common/vector.hpp"

namespace { // anonymous
NOTF_USING_NAMESPACE

/// Returns the next available SceneNodeId.
/// Is thread-safe and ever-increasing.
SceneNodeId next_id()
{
    static std::atomic<SceneNodeId::underlying_t> g_nextID(1);
    return g_nextID++;
}

} // namespace

NOTF_OPEN_NAMESPACE

//====================================================================================================================//

SceneNode::ChildContainer::~ChildContainer() {}

//====================================================================================================================//

SceneNode::SceneNode(const Token&, std::unique_ptr<ChildContainer> container)
    : m_children(std::move(container)), m_id(next_id())
{
    log_trace << "Created SceneNode #" << m_id;
}

SceneNode::~SceneNode()
{
    if (m_name.empty()) {
        log_trace << "Destroying SceneNode #" << m_id;
    }
    else {
        log_trace << "Destroying SceneNode \"" << m_name << "\"";
    }

    m_children->_destroy();
    m_children.reset();

    if (m_parent) {
        m_parent->_remove_child(this);
    }
    m_parent = nullptr;
}

bool SceneNode::has_ancestor(const SceneNode* ancestor) const
{
    if (!ancestor) {
        return false;
    }

    const SceneNode* parent = m_parent;
    while (parent) {
        if (parent == ancestor) {
            return true;
        }
        parent = parent->m_parent;
    }
    return false;
}

risky_ptr<SceneNode> SceneNode::common_ancestor(SceneNode* other)
{
    if (this == other) {
        return this;
    }

    SceneNode* first = this;
    SceneNode* second = other;

    std::unordered_set<SceneNode*> known_ancestors = {first, second};
    while (1) {
        if (first) {
            first = first->m_parent;
            if (known_ancestors.count(first)) {
                return first;
            }
            known_ancestors.insert(first);
        }
        if (second) {
            second = second->m_parent;
            if (known_ancestors.count(second)) {
                return second;
            }
            known_ancestors.insert(second);
        }
    }
}

const std::string& SceneNode::set_name(std::string name)
{
    if (name != m_name) {
        m_name = std::move(name);
        on_name_changed(m_name);
    }
    return m_name;
}

void SceneNode::_set_parent(SceneNode* parent, bool notify_old)
{
    if (parent == m_parent) {
        return;
    }

    if (m_parent && notify_old) {
        m_parent->_remove_child(this);
    }
    m_parent = parent;

    _update_from_parent();
    for (SceneNode* node : *m_children) {
        node->_update_from_parent();
    }

    on_parent_changed(m_parent);
}

//====================================================================================================================//

namespace detail {

EmptyNodeContainer::~EmptyNodeContainer() {}

SingleNodeContainer::~SingleNodeContainer() { node.reset(); }

NodeList::~NodeList() { nodes.clear(); }

} // namespace detail

NOTF_CLOSE_NAMESPACE
