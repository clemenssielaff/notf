#include "app/node.hpp"

#include <atomic>

#include "app/property_graph.hpp"
#include "common/log.hpp"
#include "common/string_view.hpp"

namespace { // anonymous
NOTF_USING_NAMESPACE

/// Returns the name of the next node.
/// Is thread-safe and ever-increasing.
std::string next_node_name()
{
    static std::atomic_size_t nextID(1);
    std::stringstream ss;
    ss << "Node#" << nextID++;
    return ss.str();
}

/// Creates a unique name from a proposed and a set of existing.
/// @param existing     All names that are already taken.
/// @param proposed     New proposed name, is presumably already taken.
/// @returns            New, unique name.
std::string make_unique_name(const std::set<std::string_view>& existing, const std::string& proposed)
{
    // remove all trailing numbers from the proposed name
    std::string_view proposed_name;
    {
        const auto last_char = proposed.find_last_not_of("0123456789");
        if (last_char == std::string_view::npos) {
            proposed_name = proposed;
        }
        else {
            proposed_name = std::string_view(proposed.c_str(), last_char);
        }
    }

    // create a unique name by appending trailing numbers until one is unique
    std::string result;
    result.reserve(proposed_name.size() + 1);
    result = std::string(proposed_name);
    size_t highest_postfix = 1;
    while (existing.count(result)) {
        result = std::string(proposed_name);
        result.append(std::to_string(highest_postfix++));
    }
    return result;
}

} // namespace

NOTF_OPEN_NAMESPACE

// ================================================================================================================== //

Node::node_finalized_error::~node_finalized_error() = default;

// ================================================================================================================== //

thread_local std::set<valid_ptr<const Node*>> Node::s_unfinalized_nodes = {};

Node::Node(const FactoryToken&, Scene& scene, valid_ptr<Node*> parent)
    : m_scene(scene), m_parent(parent), m_name(_create_name())
{
    log_trace << "Created \"" << name() << "\"";
}

Node::~Node()
{
#ifdef NOTF_DEBUG
    // all children should be removed with their parent
    for (const auto& child : m_children) {
        NOTF_ASSERT(child.raw().use_count() == 1);
    }
#endif

    _finalize();

    log_trace << "Destroying \"" << name() << "\"";
    SceneGraph::Access<Node>::remove_dirty(graph(), this);
}

const std::string& Node::set_name(const std::string& name)
{
    if (!m_name->set_value(name)) {
        log_warning << "Could not validate new name \"" << name << "\" for Node \"" << m_name->value() << "\"";
    }
    return m_name->value();
}

bool Node::has_ancestor(const valid_ptr<const Node*> ancestor) const
{
    // lock the SceneGraph hierarchy
    NOTF_MUTEX_GUARD(SceneGraph::Access<Node>::mutex(graph()));

    valid_ptr<const Node*> next = m_parent;
    while (next != next->m_parent) {
        if (next == ancestor) {
            return true;
        }
        next = next->m_parent;
    }
    return false;
}

NodeHandle<Node> Node::common_ancestor(valid_ptr<Node*> other)
{
    if (this == other) {
        return NodeHandle<Node>(this);
    }

    // lock the SceneGraph hierarchy
    NOTF_MUTEX_GUARD(SceneGraph::Access<Node>::mutex(graph()));

    valid_ptr<Node*> first = this;
    valid_ptr<Node*> second = other;
    Node* result = nullptr;
    std::unordered_set<valid_ptr<Node*>, pointer_hash<valid_ptr<Node*>>> known_ancestors = {first, second};
    while (1) {
        first = first->m_parent;
        if (known_ancestors.count(first)) {
            result = first;
            break;
        }
        known_ancestors.insert(first);

        second = second->m_parent;
        if (known_ancestors.count(second)) {
            result = second;
            break;
        }
        known_ancestors.insert(second);
    }
    NOTF_ASSERT(result);

    // if the result is a scene root node, we need to make sure that it is in fact the root of BOTH nodes
    if (result->m_parent == result && (!has_ancestor(result) || !other->has_ancestor(result))) {
        notf_throw_format(hierarchy_error, "Nodes \"{}\" and \"{}\" are not part of the same hierarchy", name(),
                          other->name());
    }
    return NodeHandle<Node>(result);
}

bool Node::is_in_front() const
{
    // lock the SceneGraph hierarchy
    NOTF_MUTEX_GUARD(SceneGraph::Access<Node>::mutex(graph()));

    const NodeContainer& siblings = m_parent->_read_children();
    NOTF_ASSERT(!siblings.empty());
    return siblings.front() == this;
}

bool Node::is_in_back() const
{
    // lock the SceneGraph hierarchy
    NOTF_MUTEX_GUARD(SceneGraph::Access<Node>::mutex(graph()));

    const NodeContainer& siblings = m_parent->_read_children();
    NOTF_ASSERT(!siblings.empty());
    return siblings.back() == this;
}

bool Node::is_in_front_of(const valid_ptr<Node*> sibling) const
{
    // lock the SceneGraph hierarchy
    NOTF_MUTEX_GUARD(SceneGraph::Access<Node>::mutex(graph()));

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

bool Node::is_behind(const valid_ptr<Node*> sibling) const
{
    // lock the SceneGraph hierarchy
    NOTF_MUTEX_GUARD(SceneGraph::Access<Node>::mutex(graph()));

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

void Node::stack_front()
{
    // lock the SceneGraph hierarchy
    NOTF_MUTEX_GUARD(SceneGraph::Access<Node>::mutex(graph()));

    // early out to avoid creating unnecessary deltas
    if (is_in_front()) {
        return;
    }

    NodeContainer& siblings = m_parent->_write_children();
    siblings.stack_front(this);
}

void Node::stack_back()
{
    // lock the SceneGraph hierarchy
    NOTF_MUTEX_GUARD(SceneGraph::Access<Node>::mutex(graph()));

    // early out to avoid creating unnecessary deltas
    if (is_in_back()) {
        return;
    }

    NodeContainer& siblings = m_parent->_write_children();
    siblings.stack_back(this);
}

void Node::stack_before(const valid_ptr<Node*> sibling)
{
    // lock the SceneGraph hierarchy
    NOTF_MUTEX_GUARD(SceneGraph::Access<Node>::mutex(graph()));

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

void Node::stack_behind(const valid_ptr<Node*> sibling)
{
    // lock the SceneGraph hierarchy
    NOTF_MUTEX_GUARD(SceneGraph::Access<Node>::mutex(graph()));

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

NodePropertyPtr Node::_property(const Path& path)
{
    if (path.is_empty()) {
        notf_throw_format(Path::path_error, "Cannot query a Property with an empty path");
    }
    if (path.size() > 1 && path.is_node()) {
        notf_throw_format(Path::path_error, "Path \"{}\" does not refer to a Property of Node \"{}\"", path.to_string(),
                          name());
    }
    if (path.is_absolute()) {
        notf_throw_format(Path::path_error, "Path \"{}\" cannot be used to query a Property of Node \"{}\"",
                          path.to_string(), name());
        // TODO: accept absolute paths if it refers to this node
    }

    // lock the SceneGraph hierarchy
    NOTF_MUTEX_GUARD(SceneGraph::Access<Node>::mutex(graph()));

    NodePropertyPtr result;
    _property(path, 0, result);
    return result;
}

void Node::_property(const Path& path, const uint index, NodePropertyPtr& result)
{
    NOTF_ASSERT(SceneGraph::Access<Node>::mutex(graph()).is_locked_by_this_thread());
    NOTF_ASSERT(index < path.size());

    // if this is the last step on the path, try to find the property
    if (index + 1 == path.size()) {
        auto it = m_properties.find(path[index]);
        if (it == m_properties.end()) {
            result.reset();
        }
        else {
            result = it->second;
        }
    }

    // if this isn't the last step yet, continue the search from the next child node
    else {
        NodePtr child = _read_children().get(path[index]).lock();
        NOTF_ASSERT(child);
        child->_property(path, index + 1, result);
    }
}

NodePtr Node::_node(const Path& path)
{
    if (path.is_empty()) {
        notf_throw_format(Path::path_error, "Cannot query a Node with an empty path");
    }
    if (path.size() > 1 && path.is_property()) { // TODO: should one-component paths always be node AND property?
        notf_throw_format(Path::path_error, "Path \"{}\" does not refer to a descenant of Node \"{}\"",
                          path.to_string(), name());
    }
    if (path.is_absolute()) {
        notf_throw_format(Path::path_error, "Path \"{}\" cannot be used to query descenant of Node \"{}\"",
                          path.to_string(), name());
        // TODO: accept absolute paths if it refers to this node
    }

    // lock the SceneGraph hierarchy
    NOTF_MUTEX_GUARD(SceneGraph::Access<Node>::mutex(graph()));

    NodePtr result;
    _node(path, 0, result);
    return result;
}

void Node::_node(const Path& path, const uint index, NodePtr& result)
{
    NOTF_ASSERT(SceneGraph::Access<Node>::mutex(graph()).is_locked_by_this_thread());
    NOTF_ASSERT(index < path.size());

    // find the next node in the path
    NodePtr child = _read_children().get(path[index]).lock();
    NOTF_ASSERT(child);

    // if this is the last step on the path, we have found what we are looking for
    if (index + 1 == path.size()) {
        result = child;
    }

    // if this isn't the last node yet, continue the search from the child
    else {
        child->_node(path, index + 1, result);
    }
}

const NodeContainer& Node::_read_children() const
{
    // make sure the SceneGraph hierarchy is properly locked
    SceneGraph& scene_graph = graph();
    NOTF_ASSERT(SceneGraph::Access<Node>::mutex(scene_graph).is_locked_by_this_thread());

    // direct access if unfrozen or this is the event handling thread
    if (!scene_graph.is_frozen() || !scene_graph.is_frozen_by(std::this_thread::get_id())) {
        return m_children;
    }

    // if the scene is frozen by this thread, try to find an existing delta first
    if (risky_ptr<NodeContainer*> delta = Scene::Access<Node>::get_delta(m_scene, this)) {
        return *delta;
    }

    // if there is no delta, allow direct read access
    else {
        return m_children;
    }
}

NodeContainer& Node::_write_children()
{
    // make sure the SceneGraph hierarchy is properly locked
    SceneGraph& scene_graph = graph();
    NOTF_ASSERT(SceneGraph::Access<Node>::mutex(scene_graph).is_locked_by_this_thread());

    // direct access if unfrozen or the node hasn't been finalized yet
    if (!scene_graph.is_frozen() || !_is_finalized()) {
        return m_children;
    }

    // the render thread should never modify the hierarchy
    NOTF_ASSERT(!scene_graph.is_frozen_by(std::this_thread::get_id()));

    // if there is no delta yet, create a new one
    if (!Scene::Access<Node>::get_delta(m_scene, this)) {
        Scene::Access<Node>::create_delta(m_scene, this);
    }

    // always modify your actual children, not the delta
    return m_children;
}

void Node::_clear_children()
{
    // lock the SceneGraph hierarchy
    NOTF_MUTEX_GUARD(SceneGraph::Access<Node>::mutex(graph()));

    _write_children().clear();
}

valid_ptr<TypedNodeProperty<std::string>*> Node::_create_name()
{
    // register this node as being unfinalized before creating a property
    s_unfinalized_nodes.emplace(this);

    // validator function for Node names, is called every time its name changes.
    TypedNodeProperty<std::string>::Validator validator = [this](std::string& name) -> bool {

        // lock the SceneGraph hierarchy
        NOTF_MUTEX_GUARD(SceneGraph::Access<Node>::mutex(graph()));
        const NodeContainer& siblings = m_parent->_read_children();

        // create unique name
        if (siblings.contains(name)) {
            std::set<std::string_view> all_names;
            for (const auto& it : NodeContainer::Access<Node>::name_map(siblings)) {
                all_names.insert(it.first);
            }
            name = make_unique_name(all_names, name);
        }

        // update parent's child container
        if (siblings.contains(this)) {
            NodeContainer::Access<Node>::rename_child(m_parent->_write_children(), this, name);
        }

        return true; // always succeeds
    };

    TypedNodePropertyPtr<std::string> name_property
        = _create_property_impl<std::string>("name", next_node_name(), std::move(validator), /* has_body = */ false);
    return name_property.get();
}

void Node::_clean_tweaks()
{
    NOTF_ASSERT(SceneGraph::Access<Node>::mutex(graph()).is_locked_by_this_thread());
    for (auto& it : m_properties) {
        NodeProperty::Access<Node>::clear_frozen(*it.second.get());
    }
}

NOTF_CLOSE_NAMESPACE
