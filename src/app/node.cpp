#include "notf/app/node.hpp"

#include "notf/common/vector.hpp"
#include "notf/meta/log.hpp"

namespace {
NOTF_USING_NAMESPACE;

std::pair<size_t, size_t>
get_this_and_sibling_index(const std::vector<NodePtr>& siblings, const Node* caller, const NodeConstPtr& sibling_ptr)
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
    // mark this node as "unfinalized", so it can create Properties if it is a RunTimeNode
    s_unfinalized_nodes.emplace(this);

    // get your own handle (only succeeds if this instance is wrapped in a shared_ptr)
    NodeHandle my_handle;
#ifdef NOTF_DEBUG
    try {
        my_handle = NodeHandle(shared_from_this());
    }
    catch (const std::bad_weak_ptr&) {
        NOTF_ASSERT(false, "Nodes must always be managed by a shared_ptr");
    }
#else
    my_handle = NodeHandle(weak_from_this());
#endif

    // register the Node with the Graph's various maps and sets
    TheGraph::AccessFor<Node>::register_node(my_handle);
    TheGraph::AccessFor<Node>::mark_dirty(my_handle);
    TheGraph::AccessFor<Node>::mark_tweaked(std::move(my_handle));

    NOTF_LOG_TRACE("Created Node {}", m_uuid.to_string());
}

Node::~Node()
{
    TheGraph::AccessFor<Node>::unregister_node(m_uuid);
    s_unfinalized_nodes.erase(this); // just in case the constructor failed
    _clear_frozen();
    NOTF_LOG_TRACE("Destroying Node {}", m_uuid.to_string());
}

NodeHandle Node::get_parent() const { return _get_parent()->weak_from_this(); }

bool Node::has_ancestor(NodeHandle ancestor) const
{
    const NodeConstPtr ancestor_lock = ancestor.m_node.lock();
    if (ancestor_lock == nullptr) { return false; }
    const Node* ancestor_ptr = ancestor_lock.get();

    for (const Node* next = _get_parent(); next != next->_get_parent(); next = next->_get_parent()) {
        if (next == ancestor_ptr) { return true; }
    }
    return false;
}

NodeHandle Node::get_common_ancestor(NodeHandle other) const
{
    const NodeConstPtr other_lock = other.m_node.lock();
    if (other_lock == nullptr) { return {}; }
    return const_cast<Node*>(_get_common_ancestor(other_lock.get()))->weak_from_this();
}

std::string_view Node::get_name() const
{
    if (m_name.empty()) { m_name = TheGraph::AccessFor<Node>::get_name(const_cast<Node*>(this)->weak_from_this()); }
    return m_name;
}

std::string_view Node::set_name(const std::string& name)
{
    m_name = TheGraph::AccessFor<Node>::set_name(weak_from_this(), name);
    return m_name;
}

NodeHandle Node::get_child(size_t index) const
{
    const ChildList& children = _read_children();
    if (index >= children.size()) {
        NOTF_THROW(out_of_bounds, "Cannot get child Node at index {} for Node \"{}\" with {} children", index,
                   get_name(), count_children());
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
    if (is_in_front()) { return; } // early out to avoid creating unnecessary deltas
    ChildList& siblings = _get_parent()->_write_children();
    auto itr = std::find(siblings.begin(), siblings.end(), this);
    NOTF_ASSERT(itr != siblings.end());
    move_to_back(siblings, itr);
}

void Node::stack_back()
{
    if (is_in_back()) { return; } // early out to avoid creating unnecessary deltas
    ChildList& siblings = _get_parent()->_write_children();
    auto itr = std::find(siblings.begin(), siblings.end(), this);
    NOTF_ASSERT(itr != siblings.end());
    move_to_front(siblings, itr);
}

void Node::stack_before(const NodeHandle& sibling)
{
    const NodeConstPtr sibling_ptr = sibling.m_node.lock();
    if ((sibling_ptr == nullptr) || (sibling_ptr->_get_parent() != _get_parent()) || (sibling_ptr.get() == this)) {
        return;
    }
    ChildList& siblings = _get_parent()->_write_children();
    const auto [my_index, sibling_index] = get_this_and_sibling_index(siblings, this, sibling_ptr);
    move_behind_of(siblings, my_index, sibling_index);
}

void Node::stack_behind(const NodeHandle& sibling)
{
    const NodeConstPtr sibling_ptr = sibling.m_node.lock();
    if ((sibling_ptr == nullptr) || (sibling_ptr->_get_parent() != _get_parent()) || (sibling_ptr.get() == this)) {
        return;
    }
    ChildList& siblings = _get_parent()->_write_children();
    const auto [my_index, sibling_index] = get_this_and_sibling_index(siblings, this, sibling_ptr);
    move_in_front_of(siblings, my_index, sibling_index);
}

void Node::_remove_child(NodePtr child)
{
    if (child == nullptr) { return; }
    ChildList& children = _write_children();
    if (auto itr = std::find(children.begin(), children.end(), child); itr != children.end()) { children.erase(itr); }
}

const Node* Node::_get_common_ancestor(const Node* other) const
{
    if (other == this) { return this; }

    const Node* first = this;
    const Node* second = other;
    const Node* result = nullptr;
    std::unordered_set<const Node*, pointer_hash<const Node*>> known_ancestors = {first, second};
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
    NOTF_ASSERT(result);
    return result;
}

Node* Node::_get_parent() const
{
    NOTF_ASSERT(m_parent);
    return m_parent;
}

void Node::_set_parent(NodePtr new_parent)
{
    if (new_parent == nullptr) { return; }

    Node* const old_parent = _get_parent();
    if (new_parent.get() == old_parent) { return; }

    auto& new_siblings = new_parent->_write_children();
    new_siblings.emplace_back(shared_from_this());
    old_parent->_remove_child(shared_from_this());
}

const Node::ChildList& Node::_read_children(const std::thread::id thread_id) const
{
    if (ChildList* frozen_list = m_cache.load(std::memory_order_consume)) {
        if (TheGraph::is_frozen_by(thread_id)) { return *frozen_list; }
    }
    return m_children;
}

Node::ChildList& Node::_write_children()
{
    // if the graph is currently frozen and this is the first modification,
    // create a frozen copy of the current child list before changing it
    if (ChildList* frozen_list = m_cache.load(std::memory_order_relaxed); frozen_list == nullptr) {
        if (TheGraph::is_frozen()) {
            m_cache.store(new ChildList(m_children), std::memory_order_release);
            TheGraph::AccessFor<Node>::mark_tweaked(weak_from_this());
        }
    }

    // changes in the child list make this node dirty
    TheGraph::AccessFor<Node>::mark_dirty(weak_from_this());

    // always modify your actual children, not the delta
    return m_children;
}

const Node::ChildList& Node::_read_siblings() const
{
    const ChildList& siblings = _get_parent()->_read_children();
    NOTF_ASSERT(contains(siblings, shared_from_this()));
    return siblings;
}

NOTF_CLOSE_NAMESPACE
