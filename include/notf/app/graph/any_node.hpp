#pragma once

#include "notf/meta/concept.hpp"
#include "notf/meta/tuple.hpp"

#include "notf/common/bitset.hpp"
#include "notf/common/uuid.hpp"

#include "notf/app/graph/graph.hpp"
#include "notf/app/graph/node_handle.hpp"
#include "notf/app/graph/property_handle.hpp"
#include "notf/app/graph/signal.hpp"
#include "notf/app/graph/slot.hpp"

NOTF_OPEN_NAMESPACE

// graph verifier =================================================================================================== //

namespace detail {

struct GraphVerifier {

    /// Checks if node type A can be a parent of node type B
    template<class A, class B>
    static constexpr bool can_a_parent_b() noexcept {
        // both A and B must be derived from Node
        if (std::negation_v<std::conjunction<std::is_base_of<AnyNode, A>, std::is_base_of<AnyNode, B>>>) {
            return false;
        }

        // if A has a list of explicitly allowed child types, B must be in it
        if constexpr (_has_allowed_child_types<A>::value) {
            if (!is_derived_from_one_of_tuple_v<B, typename A::allowed_child_types>) { return false; }
        }
        // ... otherwise, if A has a list of explicitly forbidden child types, B must NOT be in it
        else if constexpr (_has_forbidden_child_types<A>::value) {
            if (is_derived_from_one_of_tuple_v<B, typename A::forbidden_child_types>) { return false; }
        }

        // if B has a list of explicitly allowed parent types, A must be in it
        if constexpr (_has_allowed_parent_types<B>::value) {
            if (!is_derived_from_one_of_tuple_v<A, typename B::allowed_parent_types>) { return false; }
        }
        // ... otherwise, if B has a list of explicitly forbidden parent types, A must NOT be in it
        else if constexpr (_has_forbidden_parent_types<B>::value) {
            if (is_derived_from_one_of_tuple_v<A, typename B::forbidden_parent_types>) { return false; }
        }

        return true;
    }

private:
    NOTF_CREATE_TYPE_DETECTOR(allowed_child_types);
    NOTF_CREATE_TYPE_DETECTOR(forbidden_child_types);
    NOTF_CREATE_TYPE_DETECTOR(allowed_parent_types);
    NOTF_CREATE_TYPE_DETECTOR(forbidden_parent_types);
};

} // namespace detail

// any node ========================================================================================================= //

/// Base class of all Nodes.
///
/// In notf, Nodes are usually defined by Policies that determine all Properties, Signals and Slots of the Node type as
/// well as additional type limitations like the type and number of children.
/// The AnyNode base class allows access to Properties, Signals and Slots of all Nodes by runtime name (std::string) in
/// combination with the expected type, regardless of the actual Node type. If you know the type of Node, it is faster
/// and more convenient to use the compile-time functions that use ConstStrings or StringTypes, because the compiler can
/// use them to infer the expected type.
/// Using the `AnyNode` class, it is also possible to create bindings for untyped languages like Python.
/// The set of all of a node type's Properties, Signals and Slots are called *Attributes*.
///
/// The Node interface is used internally only, meaning only through Node subclasses or by other notf objects.
/// All user-access should occur through NodeHandle instances. This way, we can rely on certain preconditions to be met
/// for the user of this interface; first and foremost that mutating methods are only called from the UI thread.
class AnyNode : public std::enable_shared_from_this<AnyNode> {

    friend Accessor<AnyNode, RootNode>;
    friend Accessor<AnyNode, Window>;
    friend Accessor<AnyNode, detail::Graph>;
    friend Accessor<AnyNode, AnyWidget>;

    // types ----------------------------------------------------------------------------------- //
public:
    /// Nested `AccessFor<T>` type.
    NOTF_ACCESS_TYPE(AnyNode);

    /// Exception thrown when you try to do something that is only allowed to do if the node hasn't been finalized yet.
    /// Node finalization is an issue only with dynamic Nodes, whose Property/Slot/Signal layout is defined
    /// programatically at runtime.
    NOTF_EXCEPTION_TYPE(FinalizedError);

    // iterator ----------------------------------------------------------------

    /// Node iterator, iterates through all children of a Node in draw-order (from back to front).
    struct Iterator {

        // methods --------------------------------------------------------- //
    public:
        /// Constructor.
        /// @param node Node at the root of the iteration.
        Iterator(AnyNodeHandle node) {
            if (node) { m_nodes.emplace_back(std::move(node)); }
        }

        /// Finds and returns the next Node in the iteration.
        /// @param node [OUT] Next Node in the iteration.
        /// @returns    True if a new Node was found.
        bool next(AnyNodeHandle& output_node);

        // fields ---------------------------------------------------------- //
    private:
        /// Stack of Iterators.
        std::vector<AnyNodeHandle> m_nodes;
    };

private:
    /// Internal reactive function that is subscribed to all REDRAW Properties and marks the Node as dirty, should one
    /// of them change.
    class RedrawObserver : public Subscriber<All> {

        // methods --------------------------------------------------------- //
    public:
        /// Value Constructor.
        /// @param node     Node owning this Subscriber.
        RedrawObserver(AnyNode& node) : m_node(node) {}

        /// Called whenever a visible Property changed its value.
        void on_next(const AnyPublisher*, ...) final { m_node._set_dirty(); }

        // fields ---------------------------------------------------------- //
    private:
        /// Node owning this Subscriber.
        AnyNode& m_node;
    };
    using RedrawObserverPtr = std::shared_ptr<RedrawObserver>;

    // flags ------------------------------------------------------------------
private:
    /// Total bumber of flags on a Node (as many as fit into a word).
    using Flags = std::bitset<bitsizeof<size_t>()>;

    /// Internal Flags.
    enum class InternalFlags : uchar {
        FINALIZED,
        ENABLED,
        VISIBLE,
        DIRTY,
        __LAST,
    };
    static_assert(to_number(InternalFlags::__LAST) <= bitset_size_v<Flags>);

    /// Number of flags reserved for internal usage.
    static constexpr const size_t s_internal_flag_count = to_number(InternalFlags::__LAST);

protected:
    /// Number of user-definable flags on this system.
    static constexpr size_t s_user_flag_count = bitset_size_v<Flags> - s_internal_flag_count;

    // data -------------------------------------------------------------------
private:
    /// Unlike event handling, which is concurrent but not parallel, rendering really happens in parallel to the UI
    /// thread. If there was no synchronization between the render- and UI-thread, we could never be certain that the
    /// Graph didn't change halfway through the rendering process, resulting in frames that depict weird half-states.
    /// Therefore, all modifications on a Node are first applied to a copy of the Node's Data, while the renderer still
    /// sees the Graph as it was when it was last "synchronized".
    ///
    /// All modifiable data of a node is packed into a single `Data` object, instead of having individual
    /// pointers to a potential modified copy of each value.
    /// Since it will be a lot more common for a single Node to be modified many times than it is for many Nodes to be
    /// modified a single time, it is advantageous to have a single unused pointer to a Data object in most Nodes and a
    /// few unneccessary copies on others, than it is to have many unused pointers on most Nodes.
    struct Data {

        /// Parent of this Node.
        valid_ptr<AnyNode*> parent;

        /// All children of this Node, ordered from back to front (later Nodes are drawn on top of earlier ones).
        std::vector<AnyNodePtr> children;

        /// Additional flags, contains both internal and user-definable flags.
        Flags flags;
    };
    using ModifiedDataPtr = std::unique_ptr<Data>;

    // methods --------------------------------------------------------------------------------- //
protected:
    /// Value constructor.
    /// @param parent   Parent of this Node.
    AnyNode(valid_ptr<AnyNode*> parent);

public:
    NOTF_NO_COPY_OR_ASSIGN(AnyNode);

    /// Destructor.
    virtual ~AnyNode();

    // identification ---------------------------------------------------------
public:
    /// Uuid of this Node.
    Uuid get_uuid() const noexcept { return m_uuid; }

    /// The Graph-unique name of this Node.
    /// @returns    Name of this Node. Is an l-value because the name of the Node may change.
    std::string get_name() const { return TheGraph()->get_name(m_uuid); }

    /// (Re-)Names the Node.
    /// If another Node with the same name already exists in the Graph, this method will append the lowest integer
    /// postfix that makes the name unique.
    /// @param name     Proposed name of the Node.
    /// @returns        Actual new name of the Node.
    std::string set_name(const std::string& name);

    /// Creates a NodeHandle of this Node.
    AnyNodeHandle handle_from_this() { return weak_from_this(); }

    // properties -------------------------------------------------------------
public:
    /// The value of a Property of this Node.
    /// @param name     Node-unique name of the Property.
    /// @returns        The value of the Property.
    /// @throws         NameError / TypeError
    template<class T>
    const T& get(const std::string& name) const {
        // can be accessed from both the UI and the render thread
        return _try_get_property<T>(name)->get();
    }

    /// Updates the value of a Property of this Node.
    /// @param name     Node-unique name of the Property.
    /// @param value    New value of the Property.
    /// @throws         NameError / TypeError
    template<class T>
    void set(const std::string& name, T&& value) {
        NOTF_ASSERT(this_thread::is_the_ui_thread());
        _try_get_property<T>(name)->set(std::forward<T>(value));
    }

    /// Allows the connection to the Property of this Node in a reactive Pipeline.
    /// @param name     Node-unique name of the Property.
    /// @throws         NameError / TypeError
    template<class T>
    PropertyHandle<T> connect_property(const std::string& name) {
        NOTF_ASSERT(this_thread::is_the_ui_thread());
        return PropertyHandle<T>(_try_get_property<T>(name));
    }

    // signals / slots --------------------------------------------------------
public:
    /// @{
    /// Manually call the requested Slot of this Node.
    /// If T is not`None`, this method takes a second argument that is passed to the Slot.
    /// The Publisher of the Slot's `on_next` call id is set to `nullptr`.
    template<class T = None>
    std::enable_if_t<std::is_same_v<T, None>> call(const std::string& name) {
        NOTF_ASSERT(this_thread::is_the_ui_thread());
        _try_get_slot<T>(name)->call();
    }
    template<class T>
    std::enable_if_t<!std::is_same_v<T, None>> call(const std::string& name, const T& value) {
        NOTF_ASSERT(this_thread::is_the_ui_thread());
        _try_get_slot<T>(name)->call(value);
    }
    /// @}

    /// Run time access to the subscriber of a Slot of this Node.
    /// Use to connect Pipelines from the outside to the Node.
    /// @param name     Node-unique name of the Slot.
    /// @returns        The requested Slot.
    /// @throws         NameError / TypeError
    template<class T = None>
    SlotHandle<T> connect_slot(const std::string& name) const {
        NOTF_ASSERT(this_thread::is_the_ui_thread());
        return _try_get_slot<T>(name);
    }

    /// Run time access to a Signal of this Node.
    /// @param name     Node-unique name of the Signal.
    /// @returns        The requested Signal.
    /// @throws         NameError / TypeError
    template<class T = None>
    SignalHandle<T> connect_signal(const std::string& name) const {
        NOTF_ASSERT(this_thread::is_the_ui_thread());
        return _try_get_signal<T>(name);
    }

    // hierarchy --------------------------------------------------------------
public:
    /// The parent of this Node.
    AnyNodeHandle get_parent() const { return _get_parent()->shared_from_this(); }

    /// Tests, if this Node is a descendant of the given ancestor.
    /// If the handle passed in is expired, the answer is always false.
    /// @param ancestor         Potential ancestor to verify.
    bool has_ancestor(const AnyNodeHandle& ancestor) const;

    /// Finds and returns the first common ancestor of two Nodes.
    /// At the latest, the RootNode is always a common ancestor.
    /// If the handle passed in is expired, the returned handle will also be expired.
    /// @param other        Other Node to find the common ancestor for.
    /// @throws HandleExpiredError  If the sibling handle has expired.
    /// @throws GraphError          If there is no common ancestor.
    AnyNodeHandle get_common_ancestor(const AnyNodeHandle& other) const;

    /// Returns the first ancestor of this Node that has a specific type (can be empty if none is found).
    /// @returns    Typed handle of the first ancestor with the requested type, can be empty if none was found.
    template<class T, typename = std::enable_if_t<std::is_base_of<AnyNode, T>::value>>
    NodeHandle<T> get_first_ancestor() const {
        if (T* node = _get_first_ancestor<T>()) {
            return std::static_pointer_cast<T>(node->shared_from_this());
        } else {
            return {};
        }
    }

    /// The number of direct children of this Node.
    size_t get_child_count() const { return _read_children().size(); }

    /// Returns a handle to a child Node at the given index.
    /// Index 0 is the node furthest back, index `size() - 1` is the child drawn at the front.
    /// @param index    Index of the Node.
    /// @returns        The requested child Node.
    /// @throws IndexError    If the index is out-of-bounds or the child Node is of the wrong type.
    AnyNodeHandle get_child(size_t index) const;
    // TODO: an iterable AnyNode::get_children

    /// Destroys this Node by deleting the owning pointer in its parent.
    /// This method is basically a destructor, make sure not to dereference this Node after this function call!
    void remove() {
        NOTF_ASSERT(this_thread::is_the_ui_thread());
        AnyNodePtr parent;
        if (auto weak_parent = _get_parent()->weak_from_this(); !weak_parent.expired()) {
            parent = weak_parent.lock();
        } else {
            // Often, a NodeOwner is stored on the parent Node itself.
            // In that case, it will be destroyed right after the parent's Node class destructor has finished, and
            // just before the next subclass' destructor begins. At that point, the `shared_ptr` wrapping the outermost
            // subclass, will have already been destroyed causing all calls to `shared_from_this` or other `shared_ptr`
            // functions to throw.
            // Luckily, we can reliably test for this by checking if the `weak_ptr` contained in any Node through its
            // inheritance of `std::enable_shared_from_this` has expired. If so, we don't have to tell the parent to
            // remove the handled child, since the parent is about to be destroyed anyway and will take down all
            // children with it.
            return;
        }
        NOTF_ASSERT(parent);
        parent->_remove_child(this);
    }

    // z-order ----------------------------------------------------------------
public:
    /// Checks if this Node is in front of all of its siblings.
    bool is_in_front() const { return _read_siblings().back().get() == this; }

    /// Checks if this Node is behind all of its siblings.
    bool is_in_back() const { return _read_siblings().front().get() == this; }

    /// Returns true if this node is stacked anywhere in front of the given sibling.
    /// Also returns false if the handle is expired, the given Node is not a sibling or it is the same as this.
    /// @param sibling  Sibling node to test against.
    bool is_before(const AnyNodeHandle& sibling) const;

    /// Returns true if this node is stacked anywhere behind the given sibling.
    /// Also returns false if the handle is expired, the given Node is not a sibling or it is the same as this.
    /// @param sibling  Sibling node to test against.
    bool is_behind(const AnyNodeHandle& sibling) const;

    /// Moves this Node in front of all of its siblings.
    void stack_front();

    /// Moves this Node behind all of its siblings.
    void stack_back();

    /// Moves this Node before a given sibling.
    /// @param sibling  Sibling to stack before.
    /// @throws HandleExpiredError  If the sibling handle has expired.
    /// @throws GraphError          If the sibling is not a sibling of this node.
    void stack_before(const AnyNodeHandle& sibling);

    /// Moves this Node behind a given sibling.
    /// @param sibling  Sibling to stack behind.
    /// @throws HandleExpiredError  If the sibling handle has expired.
    /// @throws GraphError          If the sibling is not a sibling of this node.
    void stack_behind(const AnyNodeHandle& sibling);

    // properties -------------------------------------------------------------
protected:
    /// (Re-)Defines a callback to be invoked every time the value of the Property is about to change.
    /// If the callback returns false, the update is cancelled and the old value remains.
    /// If the callback returns true, the update will proceed.
    /// Since the value is passed in by mutable reference, the calblack can modify the value without restrictions. Even
    /// if the new value ends up the same as the old, the update will proceed. Note though, that the callback will only
    /// be called if the value is initially different from the one stored in the PropertyOperator.
    template<class T>
    void _set_property_callback(const std::string& property_name, typename TypedProperty<T>::callback_t callback) {
        NOTF_ASSERT(this_thread::is_the_ui_thread());
        _try_get_property<T>(property_name)->set_callback(std::move(callback));
    }

    // signals / slots --------------------------------------------------------
protected:
    /// Internal access to a Slot of this Node.
    /// Used to forward calls to the Slot from the outside to some callback inside the Node.
    /// @param name     Node-unique name of the Slot.
    /// @returns        The requested Slot.
    /// @throws         NameError / TypeError
    template<class T>
    typename TypedSlot<T>::publisher_t _get_slot(const std::string& name) const {
        NOTF_ASSERT(this_thread::is_the_ui_thread());
        return _try_get_slot<T>(name)->get_publisher();
    }

    /// @{
    /// Emits a Signal with a given value.
    /// @param name     Name of the Signal to emit.
    /// @param value    Data to emit.
    /// @throws         NameError / TypeError
    template<class T>
    void _emit(const std::string& name, const T& value) {
        NOTF_ASSERT(this_thread::is_the_ui_thread());
        _try_get_signal<T>(name)->publish(value);
    }
    void _emit(const std::string& name) {
        NOTF_ASSERT(this_thread::is_the_ui_thread());
        _try_get_signal<None>(name)->publish();
    }
    /// @}

    // flags ------------------------------------------------------------------
protected:
    /// Tests a user defineable flag on this Node.
    /// @param index    Index of the user flag.
    /// @returns        True iff the flag is set.
    /// @throws IndexError  If index >= user flag count.
    bool _get_flag(size_t index) const;

    /// Sets or unsets a user flag.
    /// @param index    Index of the user flag.
    /// @param value    Whether to set or to unser the flag.
    /// @throws IndexError  If index >= user flag count.
    void _set_flag(size_t index, bool value = true);

    // hierarchy --------------------------------------------------------------
protected:
    /// Returns the first ancestor of this Node that has a specific type.
    /// @returns    Raw pointer to the first ancestor of this Node of a specific type, is nullptr if none was found.
    template<class T, typename = std::enable_if_t<std::is_base_of<AnyNode, T>::value>>
    T* _get_first_ancestor() const {
        for (AnyNode *current = const_cast<AnyNode*>(_get_parent()), *next = nullptr;; current = next) {
            if (T* result = dynamic_cast<T*>(current)) { return result; }
            next = current->_get_parent();
            if (next == current) { return nullptr; }
        }
    }

    /// Creates and adds a new child to this node.
    /// @param parent   Parent of the Node, must be `this` (is used for type checking).
    /// @param args     Arguments that are forwarded to the constructor of the child.
    /// @throws InternalError   If you pass anything else but `this` as the parent.
    template<class Child, class Parent, class... Args,
             class = std::enable_if_t<detail::GraphVerifier::can_a_parent_b<Parent, Child>()>>
    detail::NewNode<Child> _create_child(Parent* parent, Args&&... args) {
        NOTF_ASSERT(this_thread::is_the_ui_thread());
        if (NOTF_UNLIKELY(parent != this)) {
            NOTF_THROW(InternalError, "Node::_create_child cannot be used to create children of other Nodes.");
        }

        auto child = std::make_shared<Child>(parent, std::forward<Args>(args)...);

        { // register the new node with the graph and store it as child
            auto node = std::static_pointer_cast<AnyNode>(child);
            node->_finalize();
            node->_set_finalized();
            TheGraph::AccessFor<AnyNode>::register_node(node);
            _write_children().emplace_back(std::move(node));
        }

        return child;
    }

    /// Removes a child from this node.
    /// @param handle   Handle of the node to remove.
    void _remove_child(const AnyNode* child);

    /// Remove all children from this Node.
    void _clear_children();

    /// Changes the parent of this Node by first adding it to the new parent and then removing it from its old one.
    /// @param new_parent_handle    New parent of this Node. If it is the same as the old, this method does nothing.
    void _set_parent(AnyNodeHandle new_parent_handle);

    /// Tests if the parent of this node is of a particular type.
    template<class T>
    bool _has_parent_of_type() const {
        return dynamic_cast<T*>(_get_parent()) != nullptr;
    }

protected: // for direct subclasses only
    /// Reactive function marking this Node as dirty whenever a REDRAW Property changes its value.
    RedrawObserverPtr& _get_redraw_observer() { return m_redraw_observer; }

    /// Whether or not this Node has been finalized or not.
    bool _is_finalized() const { return _get_internal_flag(to_number(InternalFlags::FINALIZED)); }

private:
    /// Implementation specific query of a Property, returns an empty pointer if no Property by the given name is found.
    /// @param name     Node-unique name of the Property.
    virtual AnyProperty* _get_property_impl(const std::string& /*name*/) const = 0;

    /// Implementation specific query of a Slot, returns an empty pointer if no Slot by the given name is found.
    /// @param name     Node-unique name of the Slot.
    virtual AnySlot* _get_slot_impl(const std::string& /*name*/) const = 0;

    /// Implementation specific query of a Signal, returns an empty pointer if no Signal by the given name is found.
    /// @param name     Node-unique name of the Signal.
    virtual AnySignalPtr _get_signal_impl(const std::string& /*name*/) const = 0;

    /// Calculates the combined hash value of all Properties.
    virtual size_t _calculate_property_hash(size_t result = detail::versioned_base_hash()) const = 0;

    /// Removes all modified data from all Properties.
    virtual void _clear_modified_properties() = 0;

    /// Access to the parent of this Node.
    /// Never creates a modified copy.
    AnyNode* _get_parent() const;

    /// Called on every new Node instance right after the constructor of the most derived class has finished.
    /// Unlike in the constructor, you can already create Handles to this Node.
    virtual void _finalize() {}

    /// Marks this Node as finalized.
    /// Called right after this Node's `_finalize` method has returned.
    void _set_finalized();

    /// Deletes all modified data of this Node.
    void _clear_modified_data();

    /// Run time access to a Property of this Node.
    /// @param name     Node-unique name of the Property.
    /// @returns        Handle to the requested Property.
    /// @throws         NameError / TypeError
    template<class T>
    TypedProperty<T>* _try_get_property(const std::string& name) const {
        // can be accessed from both the UI and the render thread
        AnyProperty* property = _get_property_impl(name);
        if (!property) { NOTF_THROW(NameError, "Node \"{}\" has no Property called \"{}\"", get_name(), name); }

        auto* typed_property = dynamic_cast<TypedProperty<T>*>(property);
        if (!typed_property) {
            NOTF_THROW(TypeError,
                       "Property \"{}\" of Node \"{}\" is of type \"{}\", but was requested as \"{}\"", //
                       name, get_name(), property->get_type_name(), type_name<T>());
        }
        return typed_property;
    }

    /// Run time access to a Slot of this Node.
    /// @param name     Node-unique name of the Slot.
    /// @returns        The requested Slot.
    /// @throws         NameError / TypeError
    template<class T>
    TypedSlot<T>* _try_get_slot(const std::string& name) const {
        NOTF_ASSERT(this_thread::is_the_ui_thread());

        AnySlot* any_slot = _get_slot_impl(name);
        if (!any_slot) { NOTF_THROW(NameError, "Node \"{}\" has no Slot called \"{}\"", get_name(), name); }

        TypedSlot<T>* typed_slot = dynamic_cast<TypedSlot<T>*>(any_slot);
        if (!typed_slot) {
            NOTF_THROW(TypeError,
                       "Slot \"{}\" of Node \"{}\" is of type \"{}\", but was requested as \"{}\"", //
                       name, get_name(), any_slot->get_type_name(), type_name<T>());
        }
        return typed_slot;
    }

    /// Run time access to a Signal of this Node.
    /// @param name     Node-unique name of the Signal.
    /// @returns        The requested Signal.
    /// @throws         NameError / TypeError
    template<class T>
    TypedSignalPtr<T> _try_get_signal(const std::string& name) const {
        NOTF_ASSERT(this_thread::is_the_ui_thread());

        AnySignalPtr any_signal = _get_signal_impl(name);
        if (!any_signal) { NOTF_THROW(NameError, "Node \"{}\" has no Signal called \"{}\"", get_name(), name); }

        TypedSignalPtr<T> typed_signal = std::dynamic_pointer_cast<TypedSignal<T>>(any_signal);
        if (!typed_signal) {
            NOTF_THROW(TypeError,
                       "Signal \"{}\" of Node \"{}\" is of type \"{}\", but was requested as \"{}\"", //
                       name, get_name(), any_signal->get_type_name(), type_name<T>());
        }
        return typed_signal;
    }

    /// Checks if the given node is an ancestor of this node.
    /// Returns false if node is this node.
    /// Returns false if node is nullptr.
    /// @param node Node to check.
    /// @returns    True iff node is an ancestor of this node.
    bool _has_ancestor(const AnyNode* node) const;

    /// Finds and returns the first common ancestor of two Nodes.
    /// At the latest, the RootNode is always a common ancestor.
    /// @param other        Other Node to find the common ancestor for.
    /// @throws GraphError  If there is no common ancestor.
    const AnyNode* _get_common_ancestor(const AnyNode* other) const;

    /// All children of this node, orded from back to front.
    /// Never creates a modified copy.
    const std::vector<AnyNodePtr>& _read_children() const;

    /// All children of this node, orded from back to front.
    /// Will create a modified copy of the children list.
    std::vector<AnyNodePtr>& _write_children();

    /// All children of the parent.
    const std::vector<AnyNodePtr>& _read_siblings() const;

    /// Tests a flag on this Node.
    /// @param index        Index of the user flag.
    /// @returns            True iff the flag is set.
    bool _get_internal_flag(size_t index) const;

    /// Sets or unsets a flag on this Node.
    /// @param index            Index of the user flag.
    /// @param value            Whether to set or to unser the flag.
    void _set_internal_flag(size_t index, bool value = true);

    /// Creates (if necessary) and returns the modified Data for this Node.
    Data& _ensure_modified_data();

    /// Marks this Node as dirty if it is finalized.
    void _set_dirty();

    // fields ---------------------------------------------------------------------------------- //
private:
    /// Uuid of this Node.
    const Uuid m_uuid = Uuid::generate();

    /// Data that might change between the start of a frame and its end.
    Data m_data;

    /// Pointer to modified Data, should this Node have been modified since the last Graph synchronization.
    ModifiedDataPtr m_modified_data;

    /// Hash of all Property values of this Node.
    size_t m_property_hash;

    /// Combines the Property hash with the Node hashes of all children in order.
    size_t m_node_hash = 0;

    /// Reactive function marking this Node as dirty whenever a REDRAW Property changes its value.
    RedrawObserverPtr m_redraw_observer = std::make_shared<RedrawObserver>(*this);
};

// node accessors =================================================================================================== //

template<>
class Accessor<AnyNode, RootNode> {
    friend RootNode;

    static void set_finalized(AnyNode& node) { node._set_finalized(); }

    static std::vector<AnyNodePtr>& write_children(AnyNode& node) { return node._write_children(); }
};

template<>
class Accessor<AnyNode, Window> {
    friend Window;

    static void set_finalized(AnyNode& node) { node._set_finalized(); }

    /// Called in the destructor, so the Window can destroy all children (including those stored in the modified data)
    /// while its GraphicsContext is still alive.
    static void remove_children_now(AnyNode* node) {
        node->m_modified_data.reset();
        node->m_data.children.clear();
    }
};

template<>
class Accessor<AnyNode, detail::Graph> {
    friend detail::Graph;

    static void clear_modified_data(AnyNode& node) { node._clear_modified_data(); }
};

template<>
class Accessor<AnyNode, AnyWidget> {
    friend AnyWidget;

    static AnyNode* get_parent(const AnyNode& node) { return node._get_parent(); }

    static const AnyNode* get_common_ancestor(const AnyNode& node, const AnyNode* other) {
        return node._get_common_ancestor(other);
    }
};

NOTF_CLOSE_NAMESPACE
