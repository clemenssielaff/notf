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
            proposed_name = std::string_view(proposed.c_str(), last_char + 1);
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

Node::Node(FactoryToken, Scene& scene, valid_ptr<Node*> parent)
    : m_scene(scene), m_parent(parent), m_name(_create_name())
{
    log_trace << "Created \"" << get_name() << "\"";
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

    log_trace << "Destroying \"" << get_name() << "\"";
    SceneGraph::Access<Node>::remove_dirty(get_graph(), this);
}

const std::string& Node::set_name(const std::string& name)
{
    NOTF_UNUSED const bool success = m_name->set(name);
    NOTF_ASSERT(success);
    return m_name->get();
}

bool Node::has_ancestor(valid_ptr<const Node*> ancestor) const
{
    NOTF_MUTEX_GUARD(_get_hierarchy_mutex());

    valid_ptr<const Node*> next = m_parent;
    while (next != next->m_parent) {
        if (next == ancestor) {
            return true;
        }
        next = next->m_parent;
    }
    return false;
}

NodeHandle<Node> Node::get_common_ancestor(valid_ptr<Node*> other)
{
    if (this == other) {
        return NodeHandle<Node>(this);
    }

    NOTF_MUTEX_GUARD(_get_hierarchy_mutex());

    valid_ptr<Node*> first = this;
    valid_ptr<Node*> second = other;
    Node* result = nullptr;
    std::unordered_set<valid_ptr<Node*>, pointer_hash<valid_ptr<Node*>>> known_ancestors = {first, second};
    while (true) {
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
        notf_throw(hierarchy_error, "Nodes \"{}\" and \"{}\" are not part of the same hierarchy", get_name(),
                   other->get_name());
    }
    return NodeHandle<Node>(result);
}

bool Node::is_in_front() const
{
    NOTF_MUTEX_GUARD(_get_hierarchy_mutex());

    const NodeContainer& siblings = m_parent->_read_children();
    NOTF_ASSERT(!siblings.empty());
    return siblings.front() == this;
}

bool Node::is_in_back() const
{
    NOTF_MUTEX_GUARD(_get_hierarchy_mutex());

    const NodeContainer& siblings = m_parent->_read_children();
    NOTF_ASSERT(!siblings.empty());
    return siblings.back() == this;
}

bool Node::is_before(valid_ptr<const Node*> sibling) const
{
    if (this == sibling) {
        return false;
    }
    return !_is_behind(sibling);
}

bool Node::is_behind(valid_ptr<const Node*> sibling) const
{
    if (this == sibling) {
        return false;
    }
    return _is_behind(sibling);
}

void Node::stack_front()
{
    NOTF_MUTEX_GUARD(_get_hierarchy_mutex());

    // early out to avoid creating unnecessary deltas
    if (is_in_front()) {
        return;
    }

    NodeContainer& siblings = m_parent->_write_children();
    siblings.stack_front(this);
}

void Node::stack_back()
{
    NOTF_MUTEX_GUARD(_get_hierarchy_mutex());

    // early out to avoid creating unnecessary deltas
    if (is_in_back()) {
        return;
    }

    NodeContainer& siblings = m_parent->_write_children();
    siblings.stack_back(this);
}

void Node::stack_before(valid_ptr<const Node*> sibling)
{
    NOTF_MUTEX_GUARD(_get_hierarchy_mutex());

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

void Node::stack_behind(valid_ptr<const Node*> sibling)
{
    NOTF_MUTEX_GUARD(_get_hierarchy_mutex());

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

NodePropertyPtr Node::_get_property(const Path& path)
{
    if (path.is_empty()) {
        notf_throw(Path::path_error, "Cannot query a Property with an empty path");
    }
    if (!path.is_property()) {
        notf_throw(Path::path_error, "Path \"{}\" does not refer to a Property of Node \"{}\"", path.to_string(),
                   get_name());
    }

    uint offset = 0;
    if (path.is_absolute()) {
        const Path myPath = get_path();
        if (!path.begins_with(myPath)) {
            notf_throw(Path::path_error, "Absolute path \"{}\" cannot be used to query a Property of Node \"{}\"",
                       path.to_string(), get_name());
        }
        offset = narrow_cast<uint>(myPath.size());
    }

    NOTF_MUTEX_GUARD(_get_hierarchy_mutex());

    NodePropertyPtr result;
    _get_property(path, offset, result);
    return result;
}

void Node::_get_property(const Path& path, const uint index, NodePropertyPtr& result)
{
    NOTF_ASSERT(_get_hierarchy_mutex().is_locked_by_this_thread());
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
        child->_get_property(path, index + 1, result);
    }
}

NodePtr Node::_get_node(const Path& path)
{
    if (path.is_empty()) {
        notf_throw(Path::path_error, "Cannot query a Node with an empty path");
    }
    if (!path.is_node()) {
        notf_throw(Path::path_error, "Path \"{}\" does not refer to a descenant of Node \"{}\"", path.to_string(),
                   get_name());
    }

    uint offset = 0;
    if (path.is_absolute()) {
        const Path myPath = get_path();
        if (!path.begins_with(myPath)) {
            notf_throw(Path::path_error, "Path \"{}\" cannot be used to query descenant of Node \"{}\"",
                       path.to_string(), get_name());
        }
        offset = narrow_cast<uint>(myPath.size());
    }

    NOTF_MUTEX_GUARD(_get_hierarchy_mutex());

    NodePtr result;
    _get_node(path, offset, result);
    return result;
}

void Node::_get_node(const Path& path, const uint index, NodePtr& result)
{
    NOTF_ASSERT(_get_hierarchy_mutex().is_locked_by_this_thread());
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
        child->_get_node(path, index + 1, result);
    }
}

const NodeContainer& Node::_read_children() const
{
    NOTF_ASSERT(_get_hierarchy_mutex().is_locked_by_this_thread());

    // direct access if unfrozen or this is the event handling thread
    SceneGraph& scene_graph = get_graph();
    if (!scene_graph.is_frozen() || !scene_graph.is_frozen_by(std::this_thread::get_id())) {
        return m_children;
    }

    // if the scene is frozen by this thread, try to find an existing delta first
    if (risky_ptr<NodeContainer*> delta = Scene::Access<Node>::get_delta(m_scene, this)) {
        return *delta;
    }

    // if there is no delta, allow direct read access
    return m_children;
}

NodeContainer& Node::_write_children()
{
    NOTF_ASSERT(_get_hierarchy_mutex().is_locked_by_this_thread());
    NOTF_ASSERT(SceneGraph::Access<Node>::event_mutex(get_graph()).is_locked_by_this_thread());

    // direct access if unfrozen or the node hasn't been finalized yet
    SceneGraph& scene_graph = get_graph();
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
    NOTF_MUTEX_GUARD(_get_hierarchy_mutex());

    _write_children().clear();
}

valid_ptr<TypedNodeProperty<std::string>*> Node::_create_name()
{
    // register this node as being unfinalized before creating a property
    s_unfinalized_nodes.emplace(this);

    // validator function for Node names, is called every time its name changes.
    TypedNodeProperty<std::string>::Validator validator = [this](std::string& name) -> bool {
        NOTF_MUTEX_GUARD(_get_hierarchy_mutex());
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
    NOTF_ASSERT(_get_hierarchy_mutex().is_locked_by_this_thread());
    for (auto& it : m_properties) {
        NodeProperty::Access<Node>::clear_frozen(*it.second.get());
    }
}

void Node::_initialize_path(const size_t depth, std::vector<std::string>& components) const
{
    if (m_parent == this) {
        // root node
        NOTF_ASSERT(components.empty());
        components.reserve(depth);
        components.push_back(m_scene.get_name());
    }
    else {
        // ancestor node
        m_parent->_initialize_path(depth + 1, components);
        components.push_back(get_name());
    }
}

bool Node::_is_behind(valid_ptr<const Node*> sibling) const
{
    if (m_parent != sibling->m_parent) {
        notf_throw(hierarchy_error, "Cannot compare z-order of nodes \"{}\" and \"{}\", because they are not siblings.",
                   get_name(), sibling->get_name());
    }
    {
        NOTF_MUTEX_GUARD(_get_hierarchy_mutex());
        const NodeContainer& siblings = m_parent->_read_children();
        for (const auto& it : siblings) {
            if (it == this) {
                return true;
            }
            if (it == sibling) {
                return false;
            }
        }
    }
    NOTF_ASSERT(false);
    return false; // to squelch warnings
}

NOTF_CLOSE_NAMESPACE
