#include "app/scene.hpp"

#include "app/event_manager.hpp"
#include "app/io/char_event.hpp"
#include "app/io/key_event.hpp"
#include "app/io/mouse_event.hpp"
#include "app/io/state_event.hpp"
#include "app/io/window_event.hpp"
#include "app/layer.hpp"
#include "app/scene.hpp"
#include "app/window.hpp"
#include "common/log.hpp"
#include "common/vector.hpp"

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

SceneGraph::SceneGraph(Window& window) : m_window(window), m_current_state(create_state({})) {}

SceneGraph::~SceneGraph() = default;

SceneGraph::StatePtr SceneGraph::current_state()
{
    NOTF_MUTEX_GUARD(m_hierarchy_mutex);

    if (is_frozen_by(std::this_thread::get_id())) {
        NOTF_ASSERT(m_frozen_state);
        return m_frozen_state;
    }
    return m_current_state;
}

void SceneGraph::enter_state(StatePtr new_state)
{
    EventManager& event_manger = Application::instance().event_manager();
    event_manger.handle(std::make_unique<StateChangeEvent>(&m_window, std::move(new_state)));
}

PropertyGraph& SceneGraph::property_graph() const { return m_window.property_graph(); }

void SceneGraph::_register_dirty(valid_ptr<SceneNode*> node)
{
    NOTF_MUTEX_GUARD(m_hierarchy_mutex);

    if (m_dirty_nodes.empty()) {
        m_window.request_redraw();
    }
    m_dirty_nodes.insert(node);
}

void SceneGraph::_remove_dirty(valid_ptr<SceneNode*> node)
{
    NOTF_MUTEX_GUARD(m_hierarchy_mutex);

    m_dirty_nodes.erase(node);
}

void SceneGraph::_propagate_event(EventPtr&& untyped_event)
{
    static const size_t char_event = CharEvent::static_type();
    static const size_t key_event = KeyEvent::static_type();
    static const size_t mouse_event = MouseEvent::static_type();
    static const size_t window_event = WindowEvent::static_type();
    static const size_t window_resize_event = WindowResizeEvent::static_type();
    static const size_t state_change_event = StateChangeEvent::static_type();

    NOTF_MUTEX_GUARD(m_event_mutex);

    const size_t event_type = untyped_event->type();

    // MouseEvent
    if (event_type == mouse_event) {
        MouseEvent* event = static_cast<MouseEvent*>(untyped_event.get());
        for (auto& layer : m_current_state->layers()) {
            if (!layer->is_active()) {
                continue;
            }
            if (risky_ptr<Scene*> scene = layer->scene()) {
                Scene::Access<SceneGraph>::handle_event(*scene, *event);
            }
            if (event->was_handled()) { // TODO: maybe add virtual "is_handled" function? Or "is_handleable_ subtype
                break;                  // this way we wouldn't have to differentiate between Mouse, Key, CharEvents
            }                           // they are all handled the same anyway
        }
    }

    // KeyEvent
    else if (event_type == key_event) {
        KeyEvent* event = static_cast<KeyEvent*>(untyped_event.get());
        for (auto& layer : m_current_state->layers()) {
            if (!layer->is_active()) {
                continue;
            }
            if (risky_ptr<Scene*> scene = layer->scene()) {
                Scene::Access<SceneGraph>::handle_event(*scene, *event);
            }
            if (event->was_handled()) {
                break;
            }
        }
    }

    // CharEvent
    else if (event_type == char_event) {
        CharEvent* event = static_cast<CharEvent*>(untyped_event.get());
        for (auto& layer : m_current_state->layers()) {
            if (!layer->is_active()) {
                continue;
            }
            if (risky_ptr<Scene*> scene = layer->scene()) {
                Scene::Access<SceneGraph>::handle_event(*scene, *event);
            }
            if (event->was_handled()) {
                break;
            }
        }
    }

    // WindowEvent
    else if (event_type == window_event) {
        for (auto& layer : m_current_state->layers()) {
            if (!layer->is_active()) {
                continue;
            }
            if (risky_ptr<Scene*> scene = layer->scene()) {
                Scene::Access<SceneGraph>::handle_event(*scene, *untyped_event);
            }
        }
    }

    // WindowResizeEvent
    else if (event_type == window_resize_event) {
        WindowResizeEvent* event = static_cast<WindowResizeEvent*>(untyped_event.get());
        for (auto& layer : m_current_state->layers()) {
            if (!layer->is_fullscreen()) {
                continue;
            }
            if (risky_ptr<Scene*> scene = layer->scene()) {
                Scene::Access<SceneGraph>::resize_view(*scene, event->new_size);
            }
        }
    }

    // StateChangeEvent
    else if (event_type == state_change_event) {
        StateChangeEvent* event = static_cast<StateChangeEvent*>(untyped_event.get());
        _enter_state(event->new_state);
    }

    // fallback
    else {
        log_warning << "Unhandled event of type: " << event_type;
    }
}

bool SceneGraph::_freeze(const std::thread::id thread_id)
{
    NOTF_MUTEX_GUARD(m_event_mutex);

    const size_t thread_hash = hash(thread_id);

    if (m_freezing_thread == thread_hash) {
        log_warning << "Ignoring repeated freezing of SceneGraph from the same thread";
        return false;
    }
    if (m_freezing_thread != 0) {
        log_critical << "Ignoring duplicate freezing of SceneGraph from another thread";
        return false;
    }

    m_freezing_thread = thread_hash;

    {
        NOTF_MUTEX_GUARD(m_hierarchy_mutex);
        m_frozen_state = m_current_state;
    }

    return true;
}

void SceneGraph::_unfreeze(NOTF_UNUSED const std::thread::id thread_id)
{
    NOTF_MUTEX_GUARD(m_event_mutex);

    if (m_freezing_thread == 0) {
        return; // already unfrozen
    }
    NOTF_ASSERT_MSG(m_freezing_thread == hash(thread_id),
                    "Thread #{} must not unfreeze the SceneGraph, because it was frozen by a different thread (#{}).",
                    hash(thread_id), hash(m_freezing_thread));

    {
        NOTF_MUTEX_GUARD(m_hierarchy_mutex);

        // unfreeze - otherwise all modifications would just create new deltas
        m_freezing_thread = 0;

        // clear old deltas in all scenes
        for (auto it = m_scenes.begin(); it != m_scenes.end();) {
            if (ScenePtr scene = it->lock()) {
                Scene::Access<SceneGraph>::clear_delta(*scene);
                ++it;
            }

            // ...and sort out Scenes that were deleted
            else {
                it = m_scenes.erase(it);
            }
        }
    }
}

void SceneGraph::_enter_state(valid_ptr<StatePtr> state)
{
    NOTF_MUTEX_GUARD(m_hierarchy_mutex);
    if (state != m_current_state) {
        m_current_state = std::move(state);
        m_window.request_redraw();
    }
}

//====================================================================================================================//

Scene::no_graph::~no_graph() = default;

Scene::hierarchy_error::~hierarchy_error() = default;

//====================================================================================================================//

bool Scene::NodeContainer::add(valid_ptr<SceneNodePtr> node)
{
    if (contains(node->name())) {
        return false;
    }
    m_names.emplace(std::make_pair(node->name(), node.raw()));
    m_order.emplace_back(std::move(node));
    return true;
}

void Scene::NodeContainer::erase(const SceneNodePtr& node)
{
    auto it = std::find(m_order.begin(), m_order.end(), node);
    if (it != m_order.end()) {
        m_names.erase((*it)->name());
        m_order.erase(it);
    }
}

void Scene::NodeContainer::stack_front(const valid_ptr<SceneNode*> node)
{
    auto it
        = std::find_if(m_order.begin(), m_order.end(), [&](const auto& sibling) -> bool { return sibling == node; });
    NOTF_ASSERT(it != m_order.end());
    move_to_back(m_order, it); // "in front" means at the end of the vector ordered back to front
}

void Scene::NodeContainer::stack_back(const valid_ptr<SceneNode*> node)
{
    auto it
        = std::find_if(m_order.begin(), m_order.end(), [&](const auto& sibling) -> bool { return sibling == node; });
    NOTF_ASSERT(it != m_order.end());
    move_to_front(m_order, it); // "in back" means at the start of the vector ordered back to front
}

void Scene::NodeContainer::stack_before(const size_t index, const valid_ptr<SceneNode*> sibling)
{
    auto node_it = iterator_at(m_order, index);
    auto sibling_it = std::find(m_order.begin(), m_order.end(), sibling);
    if (sibling_it == m_order.end()) {
        notf_throw_format(hierarchy_error,
                          "Cannot stack node \"{}\" before node \"{}\" because the two are not siblings.",
                          (*node_it)->name(), sibling->name());
    }
    notf::move_behind_of(m_order, node_it, sibling_it);
}

void Scene::NodeContainer::stack_behind(const size_t index, const valid_ptr<SceneNode*> sibling)
{
    auto node_it = iterator_at(m_order, index);
    auto sibling_it = std::find(m_order.begin(), m_order.end(), sibling);
    if (sibling_it == m_order.end()) {
        notf_throw_format(hierarchy_error,
                          "Cannot stack node \"{}\" before node \"{}\" because the two are not siblings.",
                          (*node_it)->name(), sibling->name());
    }
    notf::move_in_front_of(m_order, node_it, sibling_it);
}

//====================================================================================================================//

Scene::Scene(const FactoryToken&, const valid_ptr<SceneGraphPtr>& graph)
    : m_graph(graph.get()), m_root(std::make_shared<RootSceneNode>(SceneNode::Access<Scene>::create_token(), *this))
{}

Scene::~Scene() = default;

size_t Scene::count_nodes() const
{
    return m_root->count_descendants() + 1; // +1 is the root node
}

void Scene::clear() { m_root->clear(); }

risky_ptr<Scene::NodeContainer*> Scene::_get_delta(valid_ptr<const SceneNode*> node)
{
    NOTF_MUTEX_GUARD(SceneGraph::Access<Scene>::mutex(*graph()));

    auto it = m_deltas.find(node);
    if (it == m_deltas.end()) {
        return nullptr;
    }
    return &it->second;
}

void Scene::_create_delta(valid_ptr<const SceneNode*> node)
{
    NOTF_MUTEX_GUARD(SceneGraph::Access<Scene>::mutex(*graph()));

    auto it = m_deltas.emplace(std::make_pair(node, SceneNode::Access<Scene>::children(*node)));
    NOTF_ASSERT(it.second);
}

void access::_Scene<SceneGraph>::clear_delta(Scene& scene)
{
#ifdef NOTF_DEBUG
    // In debug mode, I'd like to ensure that each SceneNode is deleted before its parent.
    // If the parent has been modified (creating a delta referencing all child nodes) and is then deleted, calling
    // `deltas.clear()` might end up deleting the parent node before its childen.
    // Therefore we loop over all deltas and pick out those that can safely be deleted.
    bool progress = true;
    while (!scene.m_deltas.empty() && progress) {
        progress = false;
        for (auto it = scene.m_deltas.begin(); it != scene.m_deltas.end(); ++it) {
            bool childHasDelta = false;
            const Scene::NodeContainer& container = it->second;
            for (auto& child : container) {
                if (scene.m_deltas.count(child.raw().get())) {
                    childHasDelta = true;
                    break;
                }
            }
            if (!childHasDelta) {
                scene.m_deltas.erase(it);
                progress = true;
                break;
            }
        }
    }
    // If the loop should be broken because we made no progress, a child delta references a parent...
    // Should that ever happen, something has gone seriously wrong.
    NOTF_ASSERT(progress);
#else
    scene.m_deltas.clear();
#endif
}

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

//====================================================================================================================//

NOTF_CLOSE_NAMESPACE
