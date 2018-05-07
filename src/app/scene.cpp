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

void SceneGraph::_register_dirty(valid_ptr<SceneNode*> node)
{
    NOTF_ASSERT(m_event_mutex.is_locked_by_this_thread()); // must be part of the serialized event handling

    if (m_dirty_nodes.empty()) {
        m_window.request_redraw();
    }
    m_dirty_nodes.insert(node);
}

void SceneGraph::_remove_dirty(valid_ptr<SceneNode*> node)
{
    NOTF_ASSERT(m_event_mutex.is_locked_by_this_thread()); // must be part of the serialized event handling

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
                Scene::Access<SceneGraph>(*scene).handle_event(*event);
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
                Scene::Access<SceneGraph>(*scene).handle_event(*event);
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
                Scene::Access<SceneGraph>(*scene).handle_event(*event);
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
                Scene::Access<SceneGraph>(*scene).handle_event(*untyped_event);
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
                Scene::Access<SceneGraph>(*scene).resize_view(event->new_size);
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
                    hash(thread_id), hash(m_freezing_thread.load()));

    {
        NOTF_MUTEX_GUARD(m_hierarchy_mutex);

        // unfreeze - otherwise all modifications would just create new deltas
        m_freezing_thread = 0;

        // resolve delta in all scenes
        for (auto it = m_scenes.begin(); it != m_scenes.end();) {
            if (ScenePtr scene = it->lock()) {
                Scene::Access<SceneGraph>(*scene).resolve_delta();
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

Scene::Scene(const FactoryToken&, const valid_ptr<SceneGraphPtr>& graph) : m_graph(graph.get()), m_root(_create_root())
{}

void Scene::_resolve_delta()
{
#ifdef NOTF_DEBUG
    {
        SceneGraphPtr scene_graph = graph(); // this method is called from a SceneGraph, it should exist
        NOTF_ASSERT(scene_graph && SceneGraph::Access<Scene>(*scene_graph).mutex().is_locked_by_this_thread());
    }
#endif

    // resolve the delta
    for (auto& delta_it : m_deltas) {
        ChildContainer& delta = *delta_it.second.get();
        valid_ptr<const ChildContainer*> child_id = delta_it.first;
        auto child_it = m_child_container.find(child_id);
        if (child_it == m_child_container.end()) {
            continue; // parent has already been removed - ignore
        }

        // swap the delta children with the existing ones
        ChildContainer& children = *child_it->second.get();
        children.swap(delta);

        // find old children that are removed by the delta and delete them
        for (valid_ptr<SceneNode*> child_node : delta) {
            if (!contains(children, child_node)) {
                m_deletion_deltas.erase(child_node);
                _delete_node(child_node);
            }
        }
    }

    // all nodes that are left in the deletion deltas set were created and removed again, while the scene was frozen
    // since no nodes are actually deleted from a frozen scene, we need to delete them now
    for (valid_ptr<SceneNode*> leftover_node : m_deletion_deltas) {
        _delete_node(leftover_node);
    }

    m_deltas.clear();
    m_deletion_deltas.clear();
}

void Scene::_add_node(std::unique_ptr<SceneNode> node)
{
    NOTF_MUTEX_GUARD(SceneGraph::Access<Scene>(*graph()).mutex());

    SceneNode* child_handle = node.get();
    m_nodes.emplace(std::make_pair(std::move(child_handle), std::move(node)));
}

valid_ptr<Scene::ChildContainer*> Scene::_create_child_container()
{
    std::unique_ptr<ChildContainer> container = std::make_unique<ChildContainer>();
    ChildContainer* handle = container.get();
    {
        NOTF_MUTEX_GUARD(SceneGraph::Access<Scene>(*graph()).mutex());
        m_child_container.emplace(std::make_pair(handle, std::move(container)));
    }
    return handle;
}

risky_ptr<Scene::ChildContainer*> Scene::_get_delta(valid_ptr<ChildContainer*> container)
{
    NOTF_MUTEX_GUARD(SceneGraph::Access<Scene>(*graph()).mutex());

    auto it = m_deltas.find(container);
    if (it != m_deltas.end()) {
        return it->second.get();
    }
    return nullptr;
}

valid_ptr<Scene::ChildContainer*> Scene::_create_delta(valid_ptr<ChildContainer*> container)
{
    NOTF_MUTEX_GUARD(SceneGraph::Access<Scene>(*graph()).mutex());

    std::unique_ptr<ChildContainer> delta = std::make_unique<ChildContainer>(*container);
    valid_ptr<ChildContainer*> result = delta.get();
    m_deltas.emplace(std::make_pair(container, std::move(delta)));
    return result;
}

void Scene::_remove_child_container(valid_ptr<ChildContainer*> container)
{
    NOTF_MUTEX_GUARD(SceneGraph::Access<Scene>(*graph()).mutex());

    auto container_it = m_child_container.find(container);
    NOTF_ASSERT(container_it != m_child_container.end());
    NOTF_ASSERT(container_it->second->empty()); // nodes must not outlive their children
    m_child_container.erase(container_it);
}

void Scene::_delete_node(valid_ptr<SceneNode*> node)
{
    if (NOTF_UNLIKELY(node == m_root)) {
        notf_throw(hierarchy_error, "Cannot delete the root node of a scene");
    }

    NOTF_MUTEX_GUARD(SceneGraph::Access<Scene>(*graph()).mutex());

    auto node_it = m_nodes.find(node);
    NOTF_ASSERT_MSG(node_it != m_nodes.end(), "Delta could not be resolved because the Node has already been removed");

    // delete the node here while still having its `m_nodes` entry
    // this way its children can check that this is a proper removal
    node_it->second.reset();
    m_nodes.erase(node_it);
}

RootSceneNode* Scene::_create_root()
{
    std::unique_ptr<RootSceneNode> root_node
        = std::make_unique<RootSceneNode>(SceneNode::Access<Scene>::create_token(), *this);
    RootSceneNode* ptr = root_node.get();
    m_nodes.emplace(std::make_pair(ptr, std::move(root_node)));
    return ptr;
}

Scene::~Scene()
{
#ifdef NOTF_DEBUG
    if (SceneGraphPtr scene_graph = graph()) {
        NOTF_ASSERT(!scene_graph->is_frozen());
    }
#endif

    // remove the root node
    auto it = m_nodes.find(m_root);
    NOTF_ASSERT(it != m_nodes.end());
    m_nodes.erase(it);
    NOTF_ASSERT_MSG(m_nodes.empty(), "The RootNode must be the last node to be deleted");
};

//====================================================================================================================//

SceneNode::SceneNode(const FactoryToken&, Scene& scene, valid_ptr<SceneNode*> parent)
    : m_scene(scene)
    , m_parent(parent)
    , m_children(Scene::Access<SceneNode>(scene).create_child_container())
    , m_name(next_name())
{
    log_trace << "Created \"" << m_name << "\"";
}

SceneNode::~SceneNode()
{
#ifdef NOTF_DEBUG
    if (SceneGraphPtr scene_graph = m_scene.graph()) {
        NOTF_ASSERT(!scene_graph->is_frozen());
    }
#endif

    log_trace << "Destroying \"" << m_name << "\"";

    Scene::Access<SceneNode>(m_scene).remove_child_container(m_children);
}

const SceneNode::ChildContainer& SceneNode::read_children() const
{
    // make sure the SceneGraph hierarchy is properly locked
    SceneGraphPtr scene_graph = m_scene.graph();
    NOTF_ASSERT(SceneGraph::Access<SceneNode>(*scene_graph).mutex().is_locked_by_this_thread());

    // direct access if unfrozen or this is the render thread
    if (!scene_graph->is_frozen() || scene_graph->is_frozen_by(std::this_thread::get_id())) {
        return *m_children;
    }

    // if the scene is frozen, try to find an existing delta first
    if (risky_ptr<ChildContainer*> delta = Scene::Access<SceneNode>(m_scene).get_delta(m_children)) {
        return *delta;
    }

    // if there is no delta, allow direct read access
    else {
        return *m_children;
    }
}

SceneNode::ChildContainer& SceneNode::write_children()
{
    // make sure the SceneGraph hierarchy is properly locked
    Scene::Access<SceneNode> scene_access(m_scene);
    SceneGraphPtr scene_graph = m_scene.graph();
    NOTF_ASSERT(SceneGraph::Access<SceneNode>(*scene_graph).mutex().is_locked_by_this_thread());

    // direct access if unfrozen or this is the render thread
    if (!scene_graph->is_frozen() || scene_graph->is_frozen_by(std::this_thread::get_id())) {
        return *m_children;
    }

    // if the scene is frozen, try to find an existing delta first
    if (risky_ptr<ChildContainer*> delta = scene_access.get_delta(m_children)) {
        return *delta;
    }

    // if there is no delta yet, create a new one and return it
    else {
        return *scene_access.create_delta(m_children);
    }
}

bool SceneNode::has_ancestor(const valid_ptr<const SceneNode*> ancestor) const
{
    // lock the SceneGraph hierarchy
    NOTF_MUTEX_GUARD(SceneGraph::Access<SceneNode>(*m_scene.graph()).mutex());

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
    NOTF_MUTEX_GUARD(SceneGraph::Access<SceneNode>(*m_scene.graph()).mutex());

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

const std::string& SceneNode::set_name(std::string name)
{
    if (name != m_name) {
        m_name = std::move(name);
        on_name_changed(m_name);
    }
    return m_name;
}

bool SceneNode::is_in_front() const
{
    // lock the SceneGraph hierarchy
    NOTF_MUTEX_GUARD(SceneGraph::Access<SceneNode>(*m_scene.graph()).mutex());

    const ChildContainer& siblings = m_parent->read_children();
    NOTF_ASSERT(!siblings.empty());
    return siblings.back() == this;
}

bool SceneNode::is_in_back() const
{
    // lock the SceneGraph hierarchy
    NOTF_MUTEX_GUARD(SceneGraph::Access<SceneNode>(*m_scene.graph()).mutex());

    const ChildContainer& siblings = m_parent->read_children();
    NOTF_ASSERT(!siblings.empty());
    return siblings.front() == this;
}

bool SceneNode::is_in_front_of(const valid_ptr<SceneNode*> sibling) const
{
    // lock the SceneGraph hierarchy
    NOTF_MUTEX_GUARD(SceneGraph::Access<SceneNode>(*m_scene.graph()).mutex());

    const ChildContainer& siblings = m_parent->read_children();
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
    NOTF_MUTEX_GUARD(SceneGraph::Access<SceneNode>(*m_scene.graph()).mutex());

    const ChildContainer& siblings = m_parent->read_children();
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
    NOTF_MUTEX_GUARD(SceneGraph::Access<SceneNode>(*m_scene.graph()).mutex());

    // early out to avoid creating unnecessary deltas
    if (is_in_front()) {
        return;
    }

    ChildContainer& siblings = m_parent->write_children();
    auto it = std::find(siblings.begin(), siblings.end(), this);
    NOTF_ASSERT(it != siblings.end());
    move_to_back(siblings, it); // "in front" means at the end of the vector ordered back to front
}

void SceneNode::stack_back()
{
    // lock the SceneGraph hierarchy
    NOTF_MUTEX_GUARD(SceneGraph::Access<SceneNode>(*m_scene.graph()).mutex());

    // early out to avoid creating unnecessary deltas
    if (is_in_back()) {
        return;
    }

    ChildContainer& siblings = m_parent->write_children();
    auto it = std::find(siblings.begin(), siblings.end(), this);
    NOTF_ASSERT(it != siblings.end());
    move_to_front(siblings, it); // "in back" means at the start of the vector ordered back to front
}

void SceneNode::stack_before(const valid_ptr<SceneNode*> sibling)
{
    // lock the SceneGraph hierarchy
    NOTF_MUTEX_GUARD(SceneGraph::Access<SceneNode>(*m_scene.graph()).mutex());

    size_t my_index;
    { // early out to avoid creating unnecessary deltas
        const ChildContainer& siblings = m_parent->read_children();
        auto it = std::find(siblings.begin(), siblings.end(), this);
        NOTF_ASSERT(it != siblings.end());

        my_index = static_cast<size_t>(std::distance(siblings.begin(), it)); // we only need to find the index once
        if (my_index != 0 && siblings[my_index - 1] == sibling) {
            return;
        }
    }

    ChildContainer& siblings = m_parent->write_children();
    auto sibling_it = std::find(siblings.begin(), siblings.end(), sibling);
    if (sibling_it == siblings.end()) {
        notf_throw_format(hierarchy_error, "Cannot stack node \"{}\" before node \"{}\" as the two are not siblings.",
                          name(), sibling->name());
    }
    notf::move_behind_of(siblings, iterator_at(siblings, my_index), sibling_it);
}

void SceneNode::stack_behind(const valid_ptr<SceneNode*> sibling)
{
    // lock the SceneGraph hierarchy
    NOTF_MUTEX_GUARD(SceneGraph::Access<SceneNode>(*m_scene.graph()).mutex());

    size_t my_index;
    { // early out to avoid creating unnecessary deltas
        const ChildContainer& siblings = m_parent->read_children();
        auto it = std::find(siblings.begin(), siblings.end(), this);
        NOTF_ASSERT(it != siblings.end());

        my_index = static_cast<size_t>(std::distance(siblings.begin(), it)); // we only need to find the index once
        if (my_index != siblings.size() - 1 && siblings[my_index + 1] == sibling) {
            return;
        }
    }

    ChildContainer& siblings = m_parent->write_children();
    auto sibling_it = std::find(siblings.begin(), siblings.end(), sibling);
    if (sibling_it == siblings.end()) {
        notf_throw_format(hierarchy_error, "Cannot stack node \"{}\" behind node \"{}\" as the two are not siblings.",
                          name(), sibling->name());
    }
    notf::move_in_front_of(siblings, iterator_at(siblings, my_index), sibling_it);
}

//====================================================================================================================//

RootSceneNode::~RootSceneNode() {}

NOTF_CLOSE_NAMESPACE
