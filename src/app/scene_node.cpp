#include "app/scene_node.hpp"

#include <atomic>

#include "common/log.hpp"
#include "app/properties.hpp"

namespace { // anonymous
NOTF_USING_NAMESPACE

/// Returns the name of the next scene node.
/// Is thread-safe and ever-increasing.
std::string next_name()
{
    static std::atomic_size_t g_nextID(1);
    std::stringstream ss;
    ss << "SceneNode#" << g_nextID++;
    return ss.str();
}

} // namespace

NOTF_OPEN_NAMESPACE

//====================================================================================================================//

SceneNode::no_node::~no_node() = default;

SceneNode::node_finalized::~node_finalized() = default;

//====================================================================================================================//

thread_local std::set<valid_ptr<const SceneNode*>> SceneNode::s_unfinalized_nodes = {};

SceneNode::SceneNode(const FactoryToken&, Scene& scene, valid_ptr<SceneNode*> parent)
    : m_scene(scene), m_parent(parent), m_name(next_name())
{
    // register this node as being unfinalized
    s_unfinalized_nodes.emplace(this);

    log_trace << "Created \"" << m_name << "\"";
}

SceneNode::~SceneNode()
{
#ifdef NOTF_DEBUG
    // all children should be removed with their parent
    for (const auto& child : m_children) {
        NOTF_ASSERT(child.raw().use_count() == 1);
    }
#endif

    log_trace << "Destroying \"" << m_name << "\"";
    _finalize();

    try {
        SceneGraph::Access<SceneNode>::remove_dirty(*graph(), this);
    }
    catch (const Scene::no_graph&) {
        // if the SceneGraph has already been deleted, it won't matter if this SceneNode was dirty or not
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

bool SceneNode::has_ancestor(const valid_ptr<const SceneNode*> ancestor) const
{
    // lock the SceneGraph hierarchy
    NOTF_MUTEX_GUARD(SceneGraph::Access<SceneNode>::mutex(*graph()));

    valid_ptr<const SceneNode*> next = parent();
    while (next != next->parent()) {
        if (next == ancestor) {
            return true;
        }
        next = next->parent();
    }
    return false;
}

valid_ptr<SceneNode*> SceneNode::common_ancestor(valid_ptr<SceneNode*> other)
{
    if (this == other) {
        return this;
    }

    // lock the SceneGraph hierarchy
    NOTF_MUTEX_GUARD(SceneGraph::Access<SceneNode>::mutex(*graph()));

    valid_ptr<SceneNode*> first = this;
    valid_ptr<SceneNode*> second = other;
    SceneNode* result = nullptr;
    std::unordered_set<valid_ptr<SceneNode*>> known_ancestors = {first, second};
    while (1) {
        first = first->parent();
        if (known_ancestors.count(first)) {
            result = first;
            break;
        }
        known_ancestors.insert(first);

        second = second->parent();
        if (known_ancestors.count(second)) {
            result = second;
            break;
        }
        known_ancestors.insert(second);
    }
    NOTF_ASSERT(result);

    // if the result is a scene root node, we need to make sure that it is in fact the root of BOTH nodes
    if (result->parent() == result && (!has_ancestor(result) || !other->has_ancestor(result))) {
        notf_throw_format(hierarchy_error, "Nodes \"{}\" and \"{}\" are not part of the same hierarchy", name(),
                          other->name());
    }
    return result;
}

bool SceneNode::is_in_front() const
{
    // lock the SceneGraph hierarchy
    NOTF_MUTEX_GUARD(SceneGraph::Access<SceneNode>::mutex(*graph()));

    const NodeContainer& siblings = m_parent->_read_children();
    NOTF_ASSERT(!siblings.empty());
    return siblings.front() == this;
}

bool SceneNode::is_in_back() const
{
    // lock the SceneGraph hierarchy
    NOTF_MUTEX_GUARD(SceneGraph::Access<SceneNode>::mutex(*graph()));

    const NodeContainer& siblings = m_parent->_read_children();
    NOTF_ASSERT(!siblings.empty());
    return siblings.back() == this;
}

bool SceneNode::is_in_front_of(const valid_ptr<SceneNode*> sibling) const
{
    // lock the SceneGraph hierarchy
    NOTF_MUTEX_GUARD(SceneGraph::Access<SceneNode>::mutex(*graph()));

    const NodeContainer& siblings = m_parent->_read_children();
    const size_t sibling_count = siblings.size();
    size_t my_index = 0;
    for (; my_index < sibling_count; ++my_index) {
        if (siblings[my_index] == this) {
            break;
        }
        else if (siblings[my_index] == sibling) {
            return false;
        }
    }
    NOTF_ASSERT(my_index < sibling_count);

    for (size_t sibling_index = my_index + 1; sibling_index < sibling_count; ++sibling_index) {
        if (siblings[sibling_index] == sibling) {
            return true;
        }
    }
    notf_throw_format(hierarchy_error,
                      "Cannot compare z-order of nodes \"{}\" and \"{}\", because they are not siblings.", name(),
                      sibling->name());
}

bool SceneNode::is_behind(const valid_ptr<SceneNode*> sibling) const
{
    // lock the SceneGraph hierarchy
    NOTF_MUTEX_GUARD(SceneGraph::Access<SceneNode>::mutex(*graph()));

    const NodeContainer& siblings = m_parent->_read_children();
    const size_t sibling_count = siblings.size();
    size_t sibling_index = 0;
    for (; sibling_index < sibling_count; ++sibling_index) {
        if (siblings[sibling_index] == sibling) {
            break;
        }
        else if (siblings[sibling_index] == this) {
            return false;
        }
    }

    NOTF_ASSERT(sibling_index < sibling_count);
    for (size_t my_index = sibling_index + 1; my_index < sibling_count; ++my_index) {
        if (siblings[my_index] == this) {
            return true;
        }
    }
    notf_throw_format(hierarchy_error,
                      "Cannot compare z-order of nodes \"{}\" and \"{}\", because they are not siblings.", name(),
                      sibling->name());
}

void SceneNode::stack_front()
{
    // lock the SceneGraph hierarchy
    NOTF_MUTEX_GUARD(SceneGraph::Access<SceneNode>::mutex(*graph()));

    // early out to avoid creating unnecessary deltas
    if (is_in_front()) {
        return;
    }

    NodeContainer& siblings = m_parent->_write_children();
    siblings.stack_front(this);
}

void SceneNode::stack_back()
{
    // lock the SceneGraph hierarchy
    NOTF_MUTEX_GUARD(SceneGraph::Access<SceneNode>::mutex(*graph()));

    // early out to avoid creating unnecessary deltas
    if (is_in_back()) {
        return;
    }

    NodeContainer& siblings = m_parent->_write_children();
    siblings.stack_back(this);
}

void SceneNode::stack_before(const valid_ptr<SceneNode*> sibling)
{
    // lock the SceneGraph hierarchy
    NOTF_MUTEX_GUARD(SceneGraph::Access<SceneNode>::mutex(*graph()));

    size_t my_index;
    { // early out to avoid creating unnecessary deltas
        const NodeContainer& siblings = m_parent->_read_children();
        auto it = std::find(siblings.begin(), siblings.end(), this);
        NOTF_ASSERT(it != siblings.end());

        my_index = static_cast<size_t>(std::distance(siblings.begin(), it));
        if (my_index != 0 && siblings[my_index - 1] == sibling) {
            return;
        }
    }

    NodeContainer& siblings = m_parent->_write_children();
    siblings.stack_before(my_index, sibling);
}

void SceneNode::stack_behind(const valid_ptr<SceneNode*> sibling)
{
    // lock the SceneGraph hierarchy
    NOTF_MUTEX_GUARD(SceneGraph::Access<SceneNode>::mutex(*graph()));

    size_t my_index;
    { // early out to avoid creating unnecessary deltas
        const NodeContainer& siblings = m_parent->_read_children();
        auto it = std::find(siblings.begin(), siblings.end(), this);
        NOTF_ASSERT(it != siblings.end());

        my_index = static_cast<size_t>(std::distance(siblings.begin(), it)); // we only need to find the index once
        if (my_index != siblings.size() - 1 && siblings[my_index + 1] == sibling) {
            return;
        }
    }

    NodeContainer& siblings = m_parent->_write_children();
    siblings.stack_behind(my_index, sibling);
}

const SceneNode::NodeContainer& SceneNode::_read_children() const
{
    // make sure the SceneGraph hierarchy is properly locked
    SceneGraphPtr scene_graph = graph();
    NOTF_ASSERT(SceneGraph::Access<SceneNode>::mutex(*scene_graph).is_locked_by_this_thread());

    // direct access if unfrozen or this is the event handling thread
    if (!scene_graph->is_frozen() || !scene_graph->is_frozen_by(std::this_thread::get_id())) {
        return m_children;
    }

    // if the scene is frozen by this thread, try to find an existing delta first
    if (risky_ptr<NodeContainer*> delta = Scene::Access<SceneNode>::get_delta(m_scene, this)) {
        return *delta;
    }

    // if there is no delta, allow direct read access
    else {
        return m_children;
    }
}

SceneNode::NodeContainer& SceneNode::_write_children()
{
    // make sure the SceneGraph hierarchy is properly locked
    SceneGraphPtr scene_graph = graph();
    NOTF_ASSERT(SceneGraph::Access<SceneNode>::mutex(*scene_graph).is_locked_by_this_thread());

    // direct access if unfrozen or the node hasn't been finalized yet
    if (!scene_graph->is_frozen() || !_is_finalized()) {
        return m_children;
    }

    // the render thread should never modify the hierarchy
    NOTF_ASSERT(!scene_graph->is_frozen_by(std::this_thread::get_id()));

    // if there is no delta yet, create a new one
    if (!Scene::Access<SceneNode>::get_delta(m_scene, this)) {
        Scene::Access<SceneNode>::create_delta(m_scene, this);
    }

    // always modify your actual children, not the delta
    return m_children;
}

void SceneNode::_clear_children()
{
    // lock the SceneGraph hierarchy
    NOTF_MUTEX_GUARD(SceneGraph::Access<SceneNode>::mutex(*graph()));
    _write_children().clear();
}

//====================================================================================================================//

RootSceneNode::~RootSceneNode() {}

NOTF_CLOSE_NAMESPACE
