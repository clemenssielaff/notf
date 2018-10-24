#include "notf/app/node.hpp"

#include "notf/meta/log.hpp"

#include "notf/common/mnemonic.hpp"
#include "notf/common/vector.hpp"

namespace {
NOTF_USING_NAMESPACE;

std::pair<size_t, size_t> get_this_and_sibling_index(const std::vector<NodePtr>& siblings, const Node* const caller,
                                                     const NodeConstPtr& sibling_ptr)
{
    size_t my_index, sibling_index;
    {
        const size_t sibling_count = my_index = sibling_index = siblings.size();
        uchar successes = 0;
        for (size_t itr = 0; itr < sibling_count; ++itr) {
            const auto& other = siblings[itr];
            if (other.get() == caller) {
                my_index = itr;
                ++successes;
            }
            else if (other == sibling_ptr) {
                sibling_index = itr;
                ++successes;
            }
            if (successes == 2) { break; }
        }
        NOTF_ASSERT(successes == 2);
        NOTF_ASSERT(my_index != sibling_index);
        NOTF_ASSERT(my_index != sibling_count && sibling_index != sibling_count);
    }
    return std::make_pair(my_index, sibling_index);
}

} // namespace

NOTF_OPEN_NAMESPACE

// node ============================================================================================================= //

Node::Node(valid_ptr<Node*> parent) : m_parent(raw_pointer(parent))
{
    NOTF_ASSERT(TheGraph::is_locked_by_this_thread());
    NOTF_LOG_TRACE("Creating Node {}", m_uuid.to_string());
}

Node::~Node()
{
    NOTF_ASSERT(TheGraph::is_locked_by_this_thread());

    // first, delete all children while their parent pointer is still valid
    m_children.clear();
    m_modified_data.reset();

    TheGraph::AccessFor<Node>::unregister_node(m_uuid);

#ifdef NOTF_DEBUG
    // make sure that the property observer is deleted with this node, because it has a raw reference to the node
    std::weak_ptr<PropertyObserver> weak_observer = m_property_observer;
    NOTF_ASSERT(weak_observer.lock() != nullptr);
    m_property_observer.reset();
    NOTF_ASSERT(weak_observer.expired());
#endif

    NOTF_LOG_TRACE("Removing Node {}", m_uuid.to_string());
}

std::string Node::get_name() const
{
    NOTF_GUARD(std::lock_guard(TheGraph::get_name_mutex())); // lock the Node Name Registry, so the name cannot change
    return std::string(m_name);
}

std::string Node::get_default_name() const { return number_to_mnemonic(hash(get_uuid()), /*max_syllables=*/4); }

void Node::set_name(const std::string& name)
{
    NOTF_ASSERT(TheGraph::is_locked_by_this_thread());
    m_name = TheGraph::AccessFor<Node>::set_name(shared_from_this(), name);
}

NodeHandle Node::get_parent() { return _get_parent()->shared_from_this(); }

bool Node::has_ancestor(const NodeHandle& ancestor) const
{
    const NodeConstPtr ancestor_lock = ancestor.m_node.lock();
    if (ancestor_lock == nullptr) { return false; }
    const Node* const ancestor_ptr = ancestor_lock.get();

    for (const Node* next = _get_parent(); next != next->_get_parent(); next = next->_get_parent()) {
        if (next == ancestor_ptr) { return true; }
    }
    return false;
}

NodeHandle Node::get_common_ancestor(const NodeHandle& other)
{
    const NodeConstPtr other_lock = other.m_node.lock();
    if (other_lock == nullptr) { return {}; }
    return const_cast<Node*>(_get_common_ancestor(other_lock.get()))->shared_from_this();
}

NodeHandle Node::get_child(size_t index)
{
    const ChildList& children = _read_children();
    if (index >= children.size()) {
        NOTF_THROW(OutOfBounds, "Cannot get child Node at index {} for Node \"{}\" with {} children", index, get_name(),
                   count_children());
    }
    return children[index];
}

bool Node::is_in_front() const { return _read_siblings().back().get() == this; }

bool Node::is_in_back() const { return _read_siblings().front().get() == this; }

bool Node::is_before(const NodeHandle& sibling) const
{
    if (const NodeConstPtr sibling_ptr = sibling.m_node.lock();
        (sibling_ptr != nullptr) && (sibling_ptr->_get_parent() == _get_parent()) && (sibling_ptr.get() != this)) {
        for (const NodePtr& other : _read_siblings()) {
            if (other == sibling_ptr) { return true; }
            if (other.get() == this) {
                NOTF_ASSERT(contains(_read_siblings(), sibling_ptr));
                return false;
            }
        }
    }
    return false;
}

bool Node::is_behind(const NodeHandle& sibling) const
{
    if (const NodeConstPtr sibling_ptr = sibling.m_node.lock();
        (sibling_ptr != nullptr) && (sibling_ptr->_get_parent() == _get_parent()) && (sibling_ptr.get() != this)) {
        for (const NodePtr& other : _read_siblings()) {
            if (other == sibling_ptr) { return false; }
            if (other.get() == this) {
                NOTF_ASSERT(contains(_read_siblings(), sibling_ptr));
                return true;
            }
        }
    }
    return false;
}

void Node::stack_front()
{
    if (is_in_front()) { return; } // early out to avoid creating unnecessary modified copies
    ChildList& siblings = _get_parent()->_write_children();
    auto itr = std::find(siblings.begin(), siblings.end(), this);
    NOTF_ASSERT(itr != siblings.end());
    move_to_back(siblings, itr);
}

void Node::stack_back()
{
    if (is_in_back()) { return; } // early out to avoid creating unnecessary modified copies
    ChildList& siblings = _get_parent()->_write_children();
    auto itr = std::find(siblings.begin(), siblings.end(), this);
    NOTF_ASSERT(itr != siblings.end());
    move_to_front(siblings, itr);
}

void Node::stack_before(const NodeHandle& sibling)
{
    const NodeConstPtr sibling_ptr = sibling.m_node.lock();
    if ((sibling_ptr == nullptr) || (sibling_ptr.get() == this) || sibling_ptr->_get_parent() != _get_parent()) {
        return;
    }
    ChildList& siblings = _get_parent()->_write_children();
    const auto [my_index, sibling_index] = get_this_and_sibling_index(siblings, this, sibling_ptr);
    move_behind_of(siblings, my_index, sibling_index);
}

void Node::stack_behind(const NodeHandle& sibling)
{
    const NodeConstPtr sibling_ptr = sibling.m_node.lock();
    if ((sibling_ptr == nullptr) || (sibling_ptr.get() == this) || sibling_ptr->_get_parent() != _get_parent()) {
        return;
    }
    ChildList& siblings = _get_parent()->_write_children();
    const auto [my_index, sibling_index] = get_this_and_sibling_index(siblings, this, sibling_ptr);
    move_in_front_of(siblings, my_index, sibling_index);
}

bool Node::is_user_flag_set(const size_t index) const
{
    if (index >= get_user_flag_count()) {
        NOTF_THROW(OutOfBounds,
                   "Cannot test user flag #{} of Node {}, because Nodes only have {} user-defineable flags", index,
                   get_name(), get_user_flag_count());
    }
    return _is_flag_set(index + s_internal_flag_count);
}

void Node::set_user_flag(const size_t index, const bool value)
{
    if (index >= get_user_flag_count()) {
        NOTF_THROW(OutOfBounds,
                   "Cannot test user flag #{} of Node {}, because Nodes only have {} user-defineable flags", index,
                   get_name(), get_user_flag_count());
    }
    _set_flag(index + s_internal_flag_count, value);
}

void Node::_remove_child(NodeHandle child_handle)
{
    NodePtr child = child_handle.m_node.lock();
    if (child == nullptr) { return; }

    // do not create a modified copy for finding the child
    const ChildList& read_only_children = _read_children();
    auto itr = std::find(read_only_children.begin(), read_only_children.end(), child);
    if (itr == read_only_children.end()) { return; } // not a child of this node

    // remove the child node
    const size_t child_index = static_cast<size_t>(std::distance(read_only_children.begin(), itr));
    ChildList& children = _write_children();
    NOTF_ASSERT(children.at(child_index) == child);
    children.erase(iterator_at(children, child_index));
}

const Node* Node::_get_parent(const std::thread::id thread_id) const
{
    NOTF_ASSERT(m_parent);

    if (TheGraph::is_frozen_by(thread_id)) {
        return m_parent; // the renderer always sees the unmodified parent
    }
    NOTF_ASSERT(TheGraph::is_locked_by_this_thread());

    // if there exist a modified parent, return that one instead
    if (m_modified_data) {
        NOTF_ASSERT(TheGraph::is_frozen());
        NOTF_ASSERT(m_modified_data->parent);
        return m_modified_data->parent;
    }

    // either the parent has not been modified, or the Graph is not frozen
    return m_parent;
}

void Node::_set_parent(NodeHandle new_parent_handle)
{
    NodePtr new_parent = new_parent_handle.m_node.lock();
    if (new_parent == nullptr) { return; }

    Node* const old_parent = _get_parent();
    if (new_parent.get() == old_parent) { return; }

    auto& new_siblings = new_parent->_write_children();
    new_siblings.emplace_back(shared_from_this());
    old_parent->_remove_child(shared_from_this());

    if (TheGraph::is_frozen()) {
        NOTF_ASSERT(!TheGraph::is_frozen_by(std::this_thread::get_id())); // the render thread must never modify a Node
        _ensure_modified_data().parent = new_parent.get();
    }
}

const Node* Node::_get_common_ancestor(const Node* const other) const
{
    NOTF_ASSERT(other);
    if (other == this) { return this; }

    const Node* first = this;
    const Node* second = other;
    const Node* result = nullptr;
    std::unordered_set<const Node*, pointer_hash<const Node*>> known_ancestors = {first, second};
    {
        while (true) {
            first = first->_get_parent();
            if (known_ancestors.count(first) != 0) {
                result = first;
                break;
            }
            known_ancestors.insert(first);

            second = second->_get_parent();
            if (known_ancestors.count(second) != 0) {
                result = second;
                break;
            }
            known_ancestors.insert(second);
        }
    }

    if (!result) {
        NOTF_THROW(HierarchyError, "Nodes \"{}\" and \"{}\" share no common ancestor", //
                   get_uuid().to_string(), other->get_uuid().to_string());
    }
    return result;
}

const Node::ChildList& Node::_read_children(const std::thread::id thread_id) const
{
    if (TheGraph::is_frozen_by(thread_id)) {
        return m_children; // the renderer always sees the unmodified child list
    }
    NOTF_ASSERT(TheGraph::is_locked_by_this_thread());

    // if there exist a modified child list, return that one instead
    if (m_modified_data != nullptr) {
        NOTF_ASSERT(TheGraph::is_frozen());
        return *m_modified_data->children;
    }

    // either the child list has not been modified, or the Graph is not frozen
    return m_children;
}

Node::ChildList& Node::_write_children()
{
    NOTF_ASSERT(TheGraph::is_locked_by_this_thread());

    // changes in the child list make this node dirty
    TheGraph::AccessFor<Node>::mark_dirty(shared_from_this());

    if (TheGraph::is_frozen()) {
        NOTF_ASSERT(!TheGraph::is_frozen_by(std::this_thread::get_id())); // the render thread must never modify a Node
        return *_ensure_modified_data().children;
    }

    // if the graph is not frozen, modify the child list directly
    return m_children;
}

const Node::ChildList& Node::_read_siblings() const
{
    const ChildList& siblings = _get_parent()->_read_children();
    NOTF_ASSERT(contains(siblings, shared_from_this()));
    return siblings;
}

bool Node::_is_flag_set(const size_t index, const std::thread::id thread_id) const
{
    NOTF_ASSERT(index < bitset_size_v<Flags>);

    if (TheGraph::is_frozen_by(thread_id)) {
        return m_flags[index]; // the renderer always sees the unmodified flags
    }
    NOTF_ASSERT(TheGraph::is_locked_by_this_thread());

    // if a modified flag exist, return that one instead
    if (m_modified_data != nullptr) {
        NOTF_ASSERT(TheGraph::is_frozen());
        return m_modified_data->flags[index];
    }
    else {
        return m_flags[index];
    }
}

void Node::_set_flag(const size_t index, const bool value)
{
    NOTF_ASSERT(index < bitset_size_v<Flags>);
    NOTF_ASSERT(TheGraph::is_locked_by_this_thread());

    // flag changes make this node dirty
    TheGraph::AccessFor<Node>::mark_dirty(shared_from_this());

    if (TheGraph::is_frozen()) {
        NOTF_ASSERT(!TheGraph::is_frozen_by(std::this_thread::get_id())); // the render thread must never modify a Node
        _ensure_modified_data().flags[index] = value;
    }
    else {
        m_flags[index] = value;
    }
}

Node::Data& Node::_ensure_modified_data()
{
    NOTF_ASSERT(TheGraph::is_locked_by_this_thread());

    if (m_modified_data == nullptr) { m_modified_data = std::make_unique<Data>(m_parent, m_children, m_flags); }
    return *m_modified_data;
}

NOTF_CLOSE_NAMESPACE
