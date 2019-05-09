#include "notf/app/graph/any_node.hpp"

#include "notf/meta/log.hpp"

#include "notf/common/vector.hpp"

namespace {
NOTF_USING_NAMESPACE;

std::pair<size_t, size_t> get_this_and_sibling_index(const std::vector<AnyNodePtr>& siblings,
                                                     const AnyNode* const caller, const AnyNodeConstPtr& sibling_ptr) {
    size_t my_index, sibling_index;
    {
        const size_t sibling_count = my_index = sibling_index = siblings.size();
        uchar successes = 0;
        for (size_t itr = 0; itr < sibling_count; ++itr) {
            const auto& other = siblings[itr];
            if (other.get() == caller) {
                my_index = itr;
                ++successes;
            } else if (other == sibling_ptr) {
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

// any node iterator ================================================================================================ //

bool AnyNode::Iterator::next(AnyNodeHandle& next_node) {
    while (!m_nodes.empty()) {

        // take and return the next node from the stack
        NodeHandle current = take_back(m_nodes);

        // add all direct children of the next node in reverse order
        for (size_t i = current->get_child_count(); i > 0; --i) {
            m_nodes.emplace_back(current->get_child(i - 1));
        }

        // move the next node out of the function
        next_node = std::move(current);
        return true;
    }
    return false;
}

// any node ========================================================================================================= //

AnyNode::AnyNode(valid_ptr<AnyNode*> parent) : m_data({parent, {}, Flags()}) {
    NOTF_LOG_TRACE("Creating Node {}", m_uuid.to_string());
}

AnyNode::~AnyNode() {
    NOTF_LOG_TRACE("Removing Node {}", m_uuid.to_string());

    // delete all children while their parent pointer is still valid
    m_data.children.clear();
    m_modified_data.reset();

    TheGraph::AccessFor<AnyNode>::unregister_node(m_uuid);

#ifdef NOTF_DEBUG
    // make sure that the property observer is deleted with this node, because it has a raw reference to the node
    std::weak_ptr<RedrawObserver> weak_observer = m_redraw_observer;
    NOTF_ASSERT(weak_observer.lock() != nullptr);
    m_redraw_observer.reset();
    NOTF_ASSERT(weak_observer.expired());
#endif
}

std::string AnyNode::set_name(const std::string& name) {
    NOTF_ASSERT(this_thread::is_the_ui_thread()); // only the UI thread can change the name
    return TheGraph::AccessFor<AnyNode>::set_name(m_uuid, name);
}

bool AnyNode::has_ancestor(const AnyNodeHandle& ancestor) const {
    if (const AnyNodeConstPtr ancestor_lock = AnyNodeHandle::AccessFor<AnyNode>::get_node_ptr(ancestor)) {
        return _has_ancestor(ancestor_lock.get());
    } else {
        return false;
    }
}

AnyNodeHandle AnyNode::get_common_ancestor(const AnyNodeHandle& other) const {
    const AnyNodeConstPtr other_lock = AnyNodeHandle::AccessFor<AnyNode>::get_node_ptr(other);
    if (other_lock == nullptr) { NOTF_THROW(HandleExpiredError, "Node handle is expired"); }
    return const_cast<AnyNode*>(_get_common_ancestor(other_lock.get()))->shared_from_this();
}

AnyNodeHandle AnyNode::get_child(const size_t index) const {
    const std::vector<AnyNodePtr>& children = _read_children();
    if (index >= children.size()) {
        NOTF_THROW(IndexError, "Cannot get child Node at index {} for Node \"{}\" with {} children", index, get_name(),
                   get_child_count());
    }
    return children[index];
}

bool AnyNode::is_before(const AnyNodeHandle& sibling) const {
    if (const AnyNodePtr sibling_ptr = AnyNodeHandle::AccessFor<AnyNode>::get_node_ptr(sibling);
        (sibling_ptr != nullptr) && (sibling_ptr->_get_parent() == _get_parent()) && (sibling_ptr.get() != this)) {
        for (const AnyNodePtr& other : _read_siblings()) {
            if (other == sibling_ptr) { return true; }
            if (other.get() == this) {
                NOTF_ASSERT(contains(_read_siblings(), sibling_ptr));
                return false;
            }
        }
    }
    return false;
}

bool AnyNode::is_behind(const AnyNodeHandle& sibling) const {
    if (const AnyNodePtr sibling_ptr = AnyNodeHandle::AccessFor<AnyNode>::get_node_ptr(sibling);
        (sibling_ptr != nullptr) && (sibling_ptr->_get_parent() == _get_parent()) && (sibling_ptr.get() != this)) {
        for (const AnyNodePtr& other : _read_siblings()) {
            if (other == sibling_ptr) { return false; }
            if (other.get() == this) {
                NOTF_ASSERT(contains(_read_siblings(), sibling_ptr));
                return true;
            }
        }
    }
    return false;
}

void AnyNode::stack_front() {
    NOTF_ASSERT(this_thread::is_the_ui_thread());
    if (is_in_front()) { return; } // early out to avoid creating unnecessary modified copies
    std::vector<AnyNodePtr>& sibs = _get_parent()->_write_children();
    auto itr
        = std::find_if(sibs.begin(), sibs.end(), [this](const AnyNodePtr& silbing) { return silbing.get() == this; });
    NOTF_ASSERT(itr != sibs.end());
    move_to_back(sibs, itr);
}

void AnyNode::stack_back() {
    NOTF_ASSERT(this_thread::is_the_ui_thread());
    if (is_in_back()) { return; } // early out to avoid creating unnecessary modified copies
    std::vector<AnyNodePtr>& sibs = _get_parent()->_write_children();
    auto itr
        = std::find_if(sibs.begin(), sibs.end(), [this](const AnyNodePtr& silbing) { return silbing.get() == this; });
    NOTF_ASSERT(itr != sibs.end());
    move_to_front(sibs, itr);
}

void AnyNode::stack_before(const AnyNodeHandle& sibling) {
    NOTF_ASSERT(this_thread::is_the_ui_thread());
    const AnyNodeConstPtr sibling_ptr = AnyNodeHandle::AccessFor<AnyNode>::get_node_ptr(sibling);
    if (sibling_ptr == nullptr) { NOTF_THROW(HandleExpiredError, "Node handle has expired."); }
    if (sibling_ptr.get() == this) { return; }
    if (sibling_ptr->_get_parent() != _get_parent()) {
        NOTF_THROW(GraphError, "Cannot stack Node {} before {}, because they are not siblings.", get_name(),
                   sibling.get_name());
    }

    std::vector<AnyNodePtr>& siblings = _get_parent()->_write_children();
    const auto [my_index, sibling_index] = get_this_and_sibling_index(siblings, this, sibling_ptr);
    move_behind_of(siblings, my_index, sibling_index);
}

void AnyNode::stack_behind(const AnyNodeHandle& sibling) {
    NOTF_ASSERT(this_thread::is_the_ui_thread());
    const AnyNodeConstPtr sibling_ptr = AnyNodeHandle::AccessFor<AnyNode>::get_node_ptr(sibling);
    if (sibling_ptr == nullptr) { NOTF_THROW(HandleExpiredError, "Node handle has expired."); }
    if (sibling_ptr.get() == this) { return; }
    if (sibling_ptr->_get_parent() != _get_parent()) {
        NOTF_THROW(GraphError, "Cannot stack Node {} behind {}, because they are not siblings.", get_name(),
                   sibling.get_name());
    }
    std::vector<AnyNodePtr>& siblings = _get_parent()->_write_children();
    const auto [my_index, sibling_index] = get_this_and_sibling_index(siblings, this, sibling_ptr);
    move_in_front_of(siblings, my_index, sibling_index);
}

bool AnyNode::_get_flag(const size_t index) const {
    if (index >= s_user_flag_count) {
        NOTF_THROW(IndexError, "Cannot test user flag #{} of Node {}, because Nodes only have {} user-defineable flags",
                   index, get_name(), s_user_flag_count);
    }
    return _get_internal_flag(index + s_internal_flag_count);
}

void AnyNode::_set_flag(const size_t index, const bool value) {
    NOTF_ASSERT(this_thread::is_the_ui_thread());
    if (index >= s_user_flag_count) {
        NOTF_THROW(IndexError, "Cannot test user flag #{} of Node {}, because Nodes only have {} user-defineable flags",
                   index, get_name(), s_user_flag_count);
    }
    _set_internal_flag(index + s_internal_flag_count, value);
}

void AnyNode::_remove_child(const AnyNode* child) {
    NOTF_ASSERT(child);
    NOTF_ASSERT(this_thread::is_the_ui_thread());

    // do not create a modified copy for finding the child
    const std::vector<AnyNodePtr>& read_only_children = _read_children();
    auto itr = std::find_if(read_only_children.begin(), read_only_children.end(),
                            [child](const AnyNodePtr& node) { return node.get() == child; });
    if (itr == read_only_children.end()) {
        NOTF_LOG_WARN("Cannot node {} is not a child of node {}", child->get_name(), get_name());
        return;
    }

    // remove the child node
    const size_t child_index = static_cast<size_t>(std::distance(read_only_children.begin(), itr));
    std::vector<AnyNodePtr>& children = _write_children();
    NOTF_ASSERT(children.at(child_index).get() == child);
    children.erase(iterator_at(children, child_index));
}

void AnyNode::_clear_children() {
    NOTF_ASSERT(this_thread::is_the_ui_thread());
    if (_read_children().empty()) { return; } // do not create a modified copy for testing
    _write_children().clear();
}

void AnyNode::_set_parent(AnyNodeHandle new_parent_handle) {
    NOTF_ASSERT(this_thread::is_the_ui_thread());

    AnyNodePtr new_parent = AnyNodeHandle::AccessFor<AnyNode>::get_node_ptr(new_parent_handle);
    if (new_parent == nullptr) { return; }

    AnyNode* const old_parent = _get_parent();
    if (new_parent.get() == old_parent) { return; }

    auto& new_siblings = new_parent->_write_children();
    new_siblings.emplace_back(shared_from_this());
    old_parent->_remove_child(this);

    _ensure_modified_data().parent = new_parent.get();
}

AnyNode* AnyNode::_get_parent() const {
    if (!this_thread::is_the_ui_thread()) {
        return raw_pointer(m_data.parent); // the renderer always sees the unmodified parent
    }

    // if there exist a modified parent, return that one instead
    if (m_modified_data) {
        NOTF_ASSERT(m_modified_data->parent);
        return m_modified_data->parent;
    }
    return raw_pointer(m_data.parent);
}

void AnyNode::_set_finalized() {
    // do not check whether this is the UI thread as we need this method during Application construction
    NOTF_ASSERT(!m_modified_data);
    m_data.flags[to_number(InternalFlags::FINALIZED)] = true;
}

void AnyNode::_clear_modified_data() {
    NOTF_ASSERT(this_thread::is_the_ui_thread());

    // move all modified data back onto the node
    if (m_modified_data) {
        m_data = std::move(*m_modified_data);
        m_modified_data.reset();
    }

    _set_internal_flag(to_number(InternalFlags::DIRTY), false);

    _clear_modified_properties();
}

bool AnyNode::_has_ancestor(const AnyNode* const node) const {
    if (node == nullptr) { return false; }
    const AnyNode* next = _get_parent();
    for (; next != next->_get_parent(); next = next->_get_parent()) {
        if (next == node) { return true; }
    }
    return next == node; // true if the root node itself is the ancestor in question
}

const AnyNode* AnyNode::_get_common_ancestor(const AnyNode* const other) const {
    NOTF_ASSERT(other);
    if (other == this) { return this; }

    const AnyNode* first = this;
    const AnyNode* second = other;
    const AnyNode* result = nullptr;
    bool success = false;
    std::unordered_set<const AnyNode*, pointer_hash<const AnyNode*>> known_ancestors = {first, second};
    {
        while (true) {
            first = first->_get_parent();
            if (known_ancestors.count(first) != 0) {
                result = first;
                success = other->_has_ancestor(result);
                break;
            }
            known_ancestors.insert(first);

            second = second->_get_parent();
            if (known_ancestors.count(second) != 0) {
                result = second;
                success = this->_has_ancestor(result);
                break;
            }
            known_ancestors.insert(second);
        }
    }
    if (!success) {
        NOTF_THROW(GraphError, "Nodes \"{}\" and \"{}\" share no common ancestor", //
                   get_uuid().to_string(), other->get_uuid().to_string());
    }
    return result;
}

const std::vector<AnyNodePtr>& AnyNode::_read_children() const {
    if (!this_thread::is_the_ui_thread()) {
        return m_data.children; // the renderer always sees the unmodified child list
    }

    // if there exist a modified child list, return that one instead
    if (m_modified_data) { return m_modified_data->children; }
    return m_data.children;
}

std::vector<AnyNodePtr>& AnyNode::_write_children() {
    NOTF_ASSERT(this_thread::is_the_ui_thread());
    _set_dirty(); // changes in the child list make this node dirty
    return _ensure_modified_data().children;
}

const std::vector<AnyNodePtr>& AnyNode::_read_siblings() const {
    const std::vector<AnyNodePtr>& siblings = _get_parent()->_read_children();
    NOTF_ASSERT(contains_if(siblings, [this](const AnyNodePtr& sibling) -> bool { return sibling.get() == this; }));
    return siblings;
}

bool AnyNode::_get_internal_flag(const size_t index) const {
    NOTF_ASSERT(index < bitset_size_v<Flags>);

    if (!this_thread::is_the_ui_thread()) {
        return m_data.flags[index]; // the renderer always sees the unmodified flags
    }

    // if a modified flag exist, return that one instead
    if (m_modified_data != nullptr) { return m_modified_data->flags[index]; }
    return m_data.flags[index];
}

void AnyNode::_set_internal_flag(const size_t index, const bool value) {
    NOTF_ASSERT(index < bitset_size_v<Flags>);
    NOTF_ASSERT(this_thread::is_the_ui_thread());
    if (index != to_number(InternalFlags::DIRTY)) { _set_dirty(); } // flag changes make this node dirty
    _ensure_modified_data().flags[index] = value;
}

AnyNode::Data& AnyNode::_ensure_modified_data() {
    NOTF_ASSERT(this_thread::is_the_ui_thread());

    // do not create a copy if the node hasn't even been finalized yet
    if (!_is_finalized()) { return m_data; }

    if (m_modified_data == nullptr) { m_modified_data = std::make_unique<Data>(m_data); }

    return *m_modified_data;
}

void AnyNode::_set_dirty() {
    NOTF_ASSERT(this_thread::is_the_ui_thread());
    if (!_is_finalized()) { return; }
    if (!_get_internal_flag(to_number(InternalFlags::DIRTY))) {
        _set_internal_flag(to_number(InternalFlags::DIRTY));
        TheGraph::AccessFor<AnyNode>::mark_dirty(weak_from_this());
    }
}

NOTF_CLOSE_NAMESPACE
