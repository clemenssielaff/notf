#include "app/scene.hpp"

#include <atomic>
#include <unordered_set>

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

Scene::Node::Node(const Token&, Scene& scene, valid_ptr<Node*> parent)
    : m_scene(scene), m_parent(parent), m_children(_prepare_children(scene)), m_name(next_name())
{
    log_trace << "Created \"" << m_name << "\"";
}

Scene::Node::~Node()
{
    NOTF_ASSERT(m_scene.m_mutex.is_locked_by_this_thread());

    log_trace << "Destroying \"" << m_name << "\"";

    // remove child container
    auto children_it = m_scene.m_child_container.find(m_children);
    NOTF_ASSERT(children_it != m_scene.m_child_container.end());
    NOTF_ASSERT_MSG(children_it->second->empty(), "Node \"{}\" must not outlive its child nodes", name());
    m_scene.m_child_container.erase(children_it);
}

const Scene::ChildContainer& Scene::Node::read_children() const
{
    NOTF_ASSERT(m_scene.m_mutex.is_locked_by_this_thread());

    // direct access
    if (!m_scene.is_frozen() || m_scene.is_render_thread(std::this_thread::get_id())) {
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

Scene::ChildContainer& Scene::Node::write_children()
{
    NOTF_ASSERT(m_scene.m_mutex.is_locked_by_this_thread());

    // direct access
    if (!m_scene.is_frozen() || m_scene.is_render_thread(std::this_thread::get_id())) {
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

bool Scene::Node::has_ancestor(const valid_ptr<const Node*> ancestor) const
{
    valid_ptr<const Node*> next = parent();
    while (next != next->parent()) {
        if (next == ancestor) {
            return true;
        }
        next = next->parent();
    }
    return false;
}

valid_ptr<Scene::Node*> Scene::Node::common_ancestor(valid_ptr<Node*> other)
{
    if (this == other) {
        return this;
    }

    valid_ptr<Node*> first = this;
    valid_ptr<Node*> second = other;
    Scene::Node* result = nullptr;

    std::unordered_set<valid_ptr<Node*>> known_ancestors = {first, second};
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
        notf_throw_format(hierarchy_error, "Nodes \"" << name() << "\" and \"" << other->name()
                                                      << "\" are not part of the same hierarchy");
    }
    return result;
}

const std::string& Scene::Node::set_name(std::string name)
{
    if (name != m_name) {
        m_name = std::move(name);
        //        on_name_changed(m_name);
    }
    return m_name;
}

void Scene::Node::stack_front()
{
    std::lock_guard<RecursiveMutex> lock(m_scene.m_mutex);

    { // early out to avoid creating unnecessary deltas
        const ChildContainer& siblings = m_parent->read_children();
        NOTF_ASSERT(!siblings.empty());
        if (siblings.back() == this) {
            return;
        }
    }

    ChildContainer& siblings = m_parent->write_children();
    auto it = std::find(siblings.begin(), siblings.end(), this);
    NOTF_ASSERT(it != siblings.end());
    move_to_back(siblings, it); // "in front" means at the end of the vector ordered back to front
}

void Scene::Node::stack_back()
{
    std::lock_guard<RecursiveMutex> lock(m_scene.m_mutex);

    { // early out to avoid creating unnecessary deltas
        const ChildContainer& siblings = m_parent->read_children();
        NOTF_ASSERT(!siblings.empty());
        if (siblings.front() == this) {
            return;
        }
    }

    ChildContainer& siblings = m_parent->write_children();
    auto it = std::find(siblings.begin(), siblings.end(), this);
    NOTF_ASSERT(it != siblings.end());
    move_to_front(siblings, it); // "in back" means at the start of the vector ordered back to front
}

void Scene::Node::stack_before(valid_ptr<Node*> sibling)
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
        notf_throw_format(hierarchy_error, "Cannot stack node \"" << name() << "\" before node \"" << sibling->name()
                                                                  << "\" as the two are not siblings.");
    }
    notf::move_behind_of(siblings, iterator_at(siblings, my_index), sibling_it);
}

void Scene::Node::stack_behind(valid_ptr<Node*> sibling)
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
        notf_throw_format(hierarchy_error, "Cannot stack node \"" << name() << "\" behind node \"" << sibling->name()
                                                                  << "\" as the two are not siblings.");
    }
    notf::move_in_front_of(siblings, iterator_at(siblings, my_index), sibling_it);
}

valid_ptr<Scene::ChildContainer*> Scene::Node::_prepare_children(Scene& scene)
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

Scene::RootNode::RootNode(Scene& scene) : Node(Token{}, scene, this) {}

Scene::RootNode::~RootNode() {}

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

Scene::hierarchy_error::~hierarchy_error() = default;

//====================================================================================================================//

Scene::Scene(SceneManager& manager) : m_manager(manager), m_root(_create_root()) {}

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

void Scene::unfreeze(const std::thread::id thread_id)
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
        valid_ptr<const ChildContainer*> id = delta_it.first;
        ChildContainer& delta = *delta_it.second.get();

        // swap the delta children with the existing ones
        auto children_it = m_child_container.find(id);
        if (children_it == m_child_container.end()) {
            // TODO: REMOVE
            log_trace << "This actually happens - I suspected it but wasn't sure. Please remove this log entry";
            continue; // parent has already been removed - ignore
        }
        ChildContainer& children = *children_it->second.get();
        children.swap(delta);

        // find old children that are removed by the delta and delete them
        for (valid_ptr<Node*> child_id : delta) {
            if (contains(children, child_id)) {
                continue;
            }
            auto node_it = m_nodes.find(child_id);
            NOTF_ASSERT_MSG(node_it != m_nodes.end(), "Delta could not be resolved because the Node has already been "
                                                      "removed");
            node_it.value().reset();
            m_nodes.erase(node_it);
        }
    }
    m_deltas.clear();
}

Scene::RootNode* Scene::_create_root()
{
    std::unique_ptr<RootNode> root_node = std::make_unique<RootNode>(*this);
    RootNode* ptr = root_node.get();
    m_nodes.emplace(std::make_pair(ptr, std::move(root_node)));
    return ptr;
}

Scene::~Scene()
{
    std::lock_guard<RecursiveMutex> lock(m_mutex);
    NOTF_ASSERT(!is_frozen());

    // remove the root node
    auto it = m_nodes.find(m_root);
    NOTF_ASSERT(it != m_nodes.end());
    m_nodes.erase(it);
    NOTF_ASSERT_MSG(m_nodes.empty(), "The RootNode must be the last node to be deleted");
};

NOTF_CLOSE_NAMESPACE
