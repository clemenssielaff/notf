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

// any node ========================================================================================================= //

AnyNode::AnyNode(valid_ptr<AnyNode*> parent) : m_parent(raw_pointer(parent)) {
    NOTF_LOG_TRACE("Creating Node {}", m_uuid.to_string());
}

AnyNode::~AnyNode() {
    // first, delete all children while their parent pointer is still valid
    m_children.clear();
    m_modified_data.reset();

    TheGraph::AccessFor<AnyNode>::unregister_node(m_uuid);

#ifdef NOTF_DEBUG
    // make sure that the property observer is deleted with this node, because it has a raw reference to the node
    std::weak_ptr<PropertyObserver> weak_observer = m_property_observer;
    NOTF_ASSERT(weak_observer.lock() != nullptr);
    m_property_observer.reset();
    NOTF_ASSERT(weak_observer.expired());
#endif

    NOTF_LOG_TRACE("Removing Node {}", m_uuid.to_string());
}

std::string AnyNode::set_name(const std::string& name) {
    NOTF_ASSERT(this_thread::is_the_ui_thread()); // only the UI thread can change the name
    return TheGraph::AccessFor<AnyNode>::set_name(shared_from_this(), name);
}

AnyNodeHandle AnyNode::get_parent() const {
    NOTF_ASSERT(this_thread::is_the_ui_thread()); // method is const, but not thread-safe
    return _get_parent()->shared_from_this();
}

AnyNodeHandle AnyNode::get_common_ancestor(const AnyNodeHandle& other) const {
    NOTF_ASSERT(this_thread::is_the_ui_thread()); // method is const, but not thread-safe
    const AnyNodeConstPtr other_lock = AnyNodeHandle::AccessFor<AnyNode>::get_node_ptr(other);
    if (other_lock == nullptr) { return {}; }
    return const_cast<AnyNode*>(_get_common_ancestor(other_lock.get()))->shared_from_this();
}

AnyNodeHandle AnyNode::get_child(const size_t index) const {
    NOTF_ASSERT(this_thread::is_the_ui_thread()); // method is const, but not thread-safe
    const std::vector<AnyNodePtr>& children = _read_children();
    if (index >= children.size()) {
        NOTF_THROW(OutOfBounds, "Cannot get child Node at index {} for Node \"{}\" with {} children", index, get_name(),
                   get_child_count());
    }
    return children[index];
}

bool AnyNode::is_in_front() const {
    NOTF_ASSERT(this_thread::is_the_ui_thread()); // method is const, but not thread-safe
    return _read_siblings().back().get() == this;
}

bool AnyNode::is_in_back() const {
    NOTF_ASSERT(this_thread::is_the_ui_thread()); // method is const, but not thread-safe
    return _read_siblings().front().get() == this;
}

bool AnyNode::is_before(const AnyNodeHandle& sibling) const {
    NOTF_ASSERT(this_thread::is_the_ui_thread()); // method is const, but not thread-safe
    if (const AnyNodeConstPtr sibling_ptr = AnyNodeHandle::AccessFor<AnyNode>::get_node_ptr(sibling);
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
    NOTF_ASSERT(this_thread::is_the_ui_thread()); // method is const, but not thread-safe
    if (const AnyNodeConstPtr sibling_ptr = AnyNodeHandle::AccessFor<AnyNode>::get_node_ptr(sibling);
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
    if ((sibling_ptr == nullptr) || (sibling_ptr.get() == this) || sibling_ptr->_get_parent() != _get_parent()) {
        return;
    }
    std::vector<AnyNodePtr>& siblings = _get_parent()->_write_children();
    const auto [my_index, sibling_index] = get_this_and_sibling_index(siblings, this, sibling_ptr);
    move_behind_of(siblings, my_index, sibling_index);
}

void AnyNode::stack_behind(const AnyNodeHandle& sibling) {
    NOTF_ASSERT(this_thread::is_the_ui_thread());
    const AnyNodeConstPtr sibling_ptr = AnyNodeHandle::AccessFor<AnyNode>::get_node_ptr(sibling);
    if ((sibling_ptr == nullptr) || (sibling_ptr.get() == this) || sibling_ptr->_get_parent() != _get_parent()) {
        return;
    }
    std::vector<AnyNodePtr>& siblings = _get_parent()->_write_children();
    const auto [my_index, sibling_index] = get_this_and_sibling_index(siblings, this, sibling_ptr);
    move_in_front_of(siblings, my_index, sibling_index);
}

bool AnyNode::_get_flag(const size_t index) const {
    NOTF_ASSERT(this_thread::is_the_ui_thread()); // method is const, but not thread-safe
    if (index >= s_user_flag_count) {
        NOTF_THROW(OutOfBounds,
                   "Cannot test user flag #{} of Node {}, because Nodes only have {} user-defineable flags", index,
                   get_name(), s_user_flag_count);
    }
    return _get_internal_flag(index + s_internal_flag_count);
}

void AnyNode::_set_flag(const size_t index, const bool value) {
    NOTF_ASSERT(this_thread::is_the_ui_thread());
    if (index >= s_user_flag_count) {
        NOTF_THROW(OutOfBounds,
                   "Cannot test user flag #{} of Node {}, because Nodes only have {} user-defineable flags", index,
                   get_name(), s_user_flag_count);
    }
    _set_internal_flag(index + s_internal_flag_count, value);
}

void AnyNode::_remove_child(AnyNodeHandle child_handle) {
    NOTF_ASSERT(this_thread::is_the_ui_thread());
    AnyNodePtr child = AnyNodeHandle::AccessFor<AnyNode>::get_node_ptr(child_handle);
    if (child == nullptr) { return; }

    // do not create a modified copy for finding the child
    const std::vector<AnyNodePtr>& read_only_children = _read_children();
    auto itr = std::find(read_only_children.begin(), read_only_children.end(), child);
    if (itr == read_only_children.end()) { return; } // not a child of this node

    // remove the child node
    const size_t child_index = static_cast<size_t>(std::distance(read_only_children.begin(), itr));
    std::vector<AnyNodePtr>& children = _write_children();
    NOTF_ASSERT(children.at(child_index) == child);
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
    old_parent->_remove_child(shared_from_this());

    _ensure_modified_data().parent = new_parent.get();
}

AnyNode* AnyNode::_get_parent() const {
    NOTF_ASSERT(m_parent);

    if (!this_thread::is_the_ui_thread()) {
        return m_parent; // the renderer always sees the unmodified parent
    }

    // if there exist a modified parent, return that one instead
    if (m_modified_data) {
        NOTF_ASSERT(m_modified_data->parent);
        return m_modified_data->parent;
    }
    return m_parent;
}

void AnyNode::_clear_modified_data() {
    NOTF_ASSERT(this_thread::is_the_ui_thread());

    if (m_modified_data) {
        // move all modified data back onto the node
        m_parent = m_modified_data->parent;
        m_children = std::move(*m_modified_data->children.get());
        m_flags = m_modified_data->flags;

        m_modified_data.reset();
    }

    _set_internal_flag(to_number(InternalFlags::DIRTY), false);

    _clear_modified_properties();
}

bool AnyNode::_has_ancestor(const AnyNode* const ancestor) const {
    NOTF_ASSERT(this_thread::is_the_ui_thread()); // method is const, but not thread-safe
    if (ancestor == nullptr) { return false; }
    const AnyNode* next = _get_parent();
    for (; next != next->_get_parent(); next = next->_get_parent()) {
        if (next == ancestor) { return true; }
    }
    return next == ancestor; // true if the root node itself is the ancestor in question
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
        return m_children; // the renderer always sees the unmodified child list
    }

    // if there exist a modified child list, return that one instead
    if (m_modified_data) {
        NOTF_ASSERT(m_modified_data->children);
        return *m_modified_data->children;
    }
    return m_children;
}

std::vector<AnyNodePtr>& AnyNode::_write_children() {
    NOTF_ASSERT(this_thread::is_the_ui_thread());
    _mark_as_dirty(); // changes in the child list make this node dirty
    return *_ensure_modified_data().children;
}

const std::vector<AnyNodePtr>& AnyNode::_read_siblings() const {
    const std::vector<AnyNodePtr>& siblings = _get_parent()->_read_children();
    NOTF_ASSERT(contains(siblings, shared_from_this()));
    return siblings;
}

bool AnyNode::_get_internal_flag(const size_t index) const {
    NOTF_ASSERT(index < bitset_size_v<Flags>);

    if (!this_thread::is_the_ui_thread()) {
        return m_flags[index]; // the renderer always sees the unmodified flags
    }

    // if a modified flag exist, return that one instead
    if (m_modified_data != nullptr) { return m_modified_data->flags[index]; }
    return m_flags[index];
}

void AnyNode::_set_internal_flag(const size_t index, const bool value) {
    NOTF_ASSERT(index < bitset_size_v<Flags>);
    NOTF_ASSERT(this_thread::is_the_ui_thread());
    if (index != to_number(InternalFlags::DIRTY)) { _mark_as_dirty(); } // flag changes make this node dirty
    _ensure_modified_data().flags[index] = value;
}

AnyNode::Data& AnyNode::_ensure_modified_data() {
    NOTF_ASSERT(this_thread::is_the_ui_thread());
    if (m_modified_data == nullptr) { m_modified_data = std::make_unique<Data>(m_parent, m_children, m_flags); }
    return *m_modified_data;
}

void AnyNode::_mark_as_dirty() {
    if (NOTF_UNLIKELY(!_is_finalized())) { return; }
    if (!_get_internal_flag(to_number(InternalFlags::DIRTY))) {
        _set_internal_flag(to_number(InternalFlags::DIRTY));
        TheGraph::AccessFor<AnyNode>::mark_dirty(shared_from_this());
    }
}

NOTF_CLOSE_NAMESPACE
