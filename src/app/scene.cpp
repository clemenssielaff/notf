#include "app/scene.hpp"

#include <atomic>

#include "app/io/char_event.hpp"
#include "app/io/key_event.hpp"
#include "app/io/mouse_event.hpp"
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
    static std::atomic<size_t> g_nextID(1);
    std::stringstream ss;
    ss << "SceneNode#" << g_nextID++;
    return ss.str();
}

} // namespace

NOTF_OPEN_NAMESPACE

//====================================================================================================================//

SceneGraph::SceneGraph(Window& window) : m_window(window), m_current_state(create_state({})) {}

SceneGraph::~SceneGraph() = default;

void SceneGraph::enter_state(StatePtr state)
{
    m_current_state = std::move(state);
    m_window.request_redraw();
}

void SceneGraph::_register_dirty(valid_ptr<SceneNode*> node)
{
    if (m_dirty_nodes.empty()) {
        m_window.request_redraw();
    }
    m_dirty_nodes.insert(node);
}

void SceneGraph::_propagate_event(EventPtr&& untyped_event)
{
    static const size_t char_event = CharEvent::static_type();
    static const size_t key_event = KeyEvent::static_type();
    static const size_t mouse_event = MouseEvent::static_type();
    static const size_t window_event = WindowEvent::static_type();
    static const size_t window_resize_event = WindowResizeEvent::static_type();

    const size_t event_type = untyped_event->type();

    // MouseEvent
    if (event_type == mouse_event) {
        MouseEvent* event = static_cast<MouseEvent*>(untyped_event.get());
        for (auto& layer : m_current_state->layers()) {
            if (risky_ptr<Scene*> scene = layer->scene()) {
                scene->handle_event(*event);
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
            if (risky_ptr<Scene*> scene = layer->scene()) {
                scene->handle_event(*event);
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
            if (risky_ptr<Scene*> scene = layer->scene()) {
                scene->handle_event(*event);
            }
            if (event->was_handled()) {
                break;
            }
        }
    }

    // WindowEvent
    else if (event_type == window_event) {
        for (auto& layer : m_current_state->layers()) {
            if (risky_ptr<Scene*> scene = layer->scene()) {
                scene->handle_event(*untyped_event);
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
                scene->resize_view(event->new_size);
            }
        }
    }

    // fallback
    else {
        log_warning << "Unhandled event of type: " << event_type;
    }
}

//====================================================================================================================//

Scene::FreezeGuard::FreezeGuard(Scene& scene, const std::thread::id thread_id) : m_scene(&scene), m_thread_id(thread_id)
{
    if (!m_scene->freeze(m_thread_id)) {
        m_scene = nullptr;
    }
}

Scene::FreezeGuard::~FreezeGuard()
{
    if (m_scene) { // don't unfreeze if this guard tried to double-freeze the scene
        m_scene->unfreeze(m_thread_id);
    }
}

//====================================================================================================================//

Scene::Scene(SceneGraph& graph) : m_graph(graph), m_root(_create_root()) {}

bool Scene::freeze(const std::thread::id thread_id)
{
    std::lock_guard<RecursiveMutex> lock(m_mutex);
    if (m_render_thread != std::thread::id()) {
        log_warning << "Ignoring repeated freezing of Scene";
        return false;
    }
    m_render_thread = thread_id;
    return true;
}

void Scene::unfreeze(NOTF_UNUSED const std::thread::id thread_id)
{
    std::lock_guard<RecursiveMutex> lock(m_mutex);
    if (m_render_thread == std::thread::id()) {
        return;
    }
    NOTF_ASSERT_MSG(m_render_thread == thread_id,
                    "Thread #{} must not unfreeze the Scene, because it was frozen by a different thread (#{}).",
                    hash(thread_id), hash(m_render_thread));

    // unfreeze - otherwise all modifications would just create new deltas
    m_render_thread = std::thread::id();

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

void Scene::_delete_node(valid_ptr<SceneNode*> node)
{
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
    std::lock_guard<RecursiveMutex> lock(m_mutex);
    NOTF_ASSERT(!_is_frozen());

    // remove the root node
    auto it = m_nodes.find(m_root);
    NOTF_ASSERT(it != m_nodes.end());
    m_nodes.erase(it);
    NOTF_ASSERT_MSG(m_nodes.empty(), "The RootNode must be the last node to be deleted");
};

//====================================================================================================================//

SceneNode::hierarchy_error::~hierarchy_error() = default;

//====================================================================================================================//

SceneNode::SceneNode(const Token&, Scene& scene, valid_ptr<SceneNode*> parent)
    : m_scene(scene), m_parent(parent), m_children(_register_with_scene(scene)), m_name(next_name())
{
    log_trace << "Created \"" << m_name << "\"";
}

SceneNode::~SceneNode()
{
    NOTF_ASSERT(m_scene.m_mutex.is_locked_by_this_thread());

    log_trace << "Destroying \"" << m_name << "\"";

    // remove child container
    auto children_it = m_scene.m_child_container.find(m_children);
    NOTF_ASSERT(children_it != m_scene.m_child_container.end());
    NOTF_ASSERT_MSG(children_it->second->empty(), "Node \"{}\" must not outlive its child nodes", name());
    m_scene.m_child_container.erase(children_it);
}

const SceneNode::ChildContainer& SceneNode::read_children() const
{
    NOTF_ASSERT(m_scene.m_mutex.is_locked_by_this_thread());

    // direct access
    if (!m_scene._is_frozen() || m_scene._is_render_thread(std::this_thread::get_id())) {
        return *m_children;
    }

    // if the scene is frozen, try to find an existing delta first
    auto it = m_scene.m_deltas.find(m_children);
    if (it != m_scene.m_deltas.end()) {
        return *it->second.get();
    }

    // if there is no delta, allow direct read access
    else {
        return *m_children;
    }
}

SceneNode::ChildContainer& SceneNode::write_children()
{
    NOTF_ASSERT(m_scene.m_mutex.is_locked_by_this_thread());

    // direct access
    if (!m_scene._is_frozen() || m_scene._is_render_thread(std::this_thread::get_id())) {
        return *m_children;
    }

    // if the scene is frozen, try to find an existing delta first
    auto it = m_scene.m_deltas.find(m_children);
    if (it != m_scene.m_deltas.end()) {
        return *it->second.get();
    }

    // if there is no delta yet, create a new one and return it
    else {
        std::unique_ptr<ChildContainer> delta = std::make_unique<ChildContainer>(*m_children);
        ChildContainer& result = *delta.get();
        m_scene.m_deltas.emplace(std::make_pair(m_children, std::move(delta)));
        return result;
    }
}

bool SceneNode::has_ancestor(const valid_ptr<const SceneNode*> ancestor) const
{
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
    std::lock_guard<RecursiveMutex> lock(m_scene.m_mutex);
    const ChildContainer& siblings = m_parent->read_children();
    NOTF_ASSERT(!siblings.empty());
    return siblings.back() == this;
}

bool SceneNode::is_in_back() const
{
    std::lock_guard<RecursiveMutex> lock(m_scene.m_mutex);
    const ChildContainer& siblings = m_parent->read_children();
    NOTF_ASSERT(!siblings.empty());
    return siblings.front() == this;
}

bool SceneNode::is_in_front_of(const valid_ptr<SceneNode*> sibling) const
{
    std::lock_guard<RecursiveMutex> lock(m_scene.m_mutex);
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
    std::lock_guard<RecursiveMutex> lock(m_scene.m_mutex);
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
    std::lock_guard<RecursiveMutex> lock(m_scene.m_mutex);

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
    std::lock_guard<RecursiveMutex> lock(m_scene.m_mutex);

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
    std::lock_guard<RecursiveMutex> lock(m_scene.m_mutex);

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
    std::lock_guard<RecursiveMutex> lock(m_scene.m_mutex);

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

valid_ptr<SceneNode::ChildContainer*> SceneNode::_register_with_scene(Scene& scene)
{
    std::unique_ptr<ChildContainer> container = std::make_unique<ChildContainer>();
    ChildContainer* handle = container.get();
    {
        std::lock_guard<RecursiveMutex> lock(scene.m_mutex);
        scene.m_child_container.emplace(std::make_pair(handle, std::move(container)));
    }
    return handle;
}
//====================================================================================================================//

RootSceneNode::~RootSceneNode() {}

NOTF_CLOSE_NAMESPACE
