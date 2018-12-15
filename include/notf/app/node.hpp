#pragma once

#include "notf/common/bitset.hpp"
#include "notf/common/uuid.hpp"

#include "notf/app/graph.hpp"
#include "notf/app/node_handle.hpp"
#include "notf/app/property_handle.hpp"
#include "notf/app/signal.hpp"
#include "notf/app/slot.hpp"

NOTF_OPEN_NAMESPACE

// node ============================================================================================================= //

/// Base class for both RunTimeNode and CompileTimeNode.
/// The Node interface is used internally only, meaning only through Node subclasses or by other notf objects.
/// All user-access should occur through NodeHandle instances. This way, we can rely on certain preconditions to be met
/// for the user of this interface; first and foremost that mutating methods are only called from the UI thread.
class Node : public std::enable_shared_from_this<Node> {

    friend Accessor<Node, RootNode>;
    friend Accessor<Node, Window>;
    friend Accessor<Node, detail::Graph>;

    // types ----------------------------------------------------------------------------------- //
public:
    /// Nested `AccessFor<T>` type.
    NOTF_ACCESS_TYPE(Node);

    /// Exception thrown when you try to do something that is only allowed to do if the node hasn't been finalized yet.
    NOTF_EXCEPTION_TYPE(FinalizedError);

    /// If two Nodes have no common ancestor.
    NOTF_EXCEPTION_TYPE(HierarchyError);

    // iterator ----------------------------------------------------------------

    /// Node iterator, iterates through all children of a Node (visually back to front).
    struct Iterator {

        // types ----------------------------------------------------------- //
    private:
        struct Impl {
            /// Constructor.
            /// @param node         The iterated Node.
            /// @param child_count  Numer of children of the Node.
            Impl(NodeHandle node, const size_t child_count) : node(std::move(node)), end(child_count) {}

            /// The iterated Node.
            NodeHandle node;

            /// Current child index.
            size_t index = 0;

            /// One index past the last child.
            const size_t end;
        };

        // methods --------------------------------------------------------- //
    public:
        /// Constructor.
        /// @param node Node at the root of the iteration.
        Iterator(NodeHandle node) {
            const size_t child_count = node->get_child_count();
            m_iterators.emplace_back(std::move(node), child_count);
        }

        /// Finds and returns the next Node in the iteration.
        /// @param node [OUT] Next Node in the iteration.
        /// @returns    True if a new Node was found.
        bool next(NodeHandle& node) {
            while (true) {
                Impl& it = m_iterators.back();
                node = it.node;

                if (it.index == it.end) {
                    if (m_iterators.size() == 1) {
                        return false;
                    } else {
                        m_iterators.pop_back();
                        continue;
                    }
                }

                const size_t child_count = node->get_child_count();
                if (child_count > 0) { m_iterators.emplace_back(node->get_child(0), child_count); }

                return true;
            }
        }

        // fields ---------------------------------------------------------- //
    private:
        /// Stack of Iterators.
        std::vector<Impl> m_iterators;
    };

private:
    /// Internal reactive function that is subscribed to all visible Properties and marks the Node as dirty, should one
    /// of them change.
    class PropertyObserver : public Subscriber<All> {

        // methods --------------------------------------------------------- //
    public:
        /// Value Constructor.
        /// @param node     Node owning this Subscriber.
        PropertyObserver(Node& node) : m_node(node) {}

        /// Called whenever a visible Property changed its value.
        void on_next(const AnyPublisher*, ...) final { m_node._mark_as_dirty(); }

        // fields ---------------------------------------------------------- //
    private:
        /// Node owning this Subscriber.
        Node& m_node;
    };
    using PropertyObserverPtr = std::shared_ptr<PropertyObserver>;

    // flags ------------------------------------------------------------------

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
    static constexpr size_t s_internal_flag_count = to_number(InternalFlags::__LAST);

    // data -------------------------------------------------------------------

    /// Unlike event handling, which is concurrent but not parallel, rendering happens truly parallel to the UI thread.
    /// If there was no synchronization between the render- and UI-thread, we could never be certain that the Graph
    /// didn't change halfway through the rendering process, resulting in frames that depict weird half-states.
    /// Therefore, all modifications on a Node are first applied to a copy of the Node's Data, while the renderer still
    /// sees the Graph as it was when it was last "synchronized".
    /// Since it will be a lot more common for a single Node to be modified many times than it is for many Nodes to be
    /// modified a single time, it is advantageous to have a single unused pointer to data in many Nodes and a few
    /// unneccessary data copies on some, than it is to have many unused pointers on most Nodes.
    struct Data {
        /// Value Constructor.
        /// Initializes all data, so we can be sure that all of it is valid if a modified data copy exists.
        Data(valid_ptr<Node*> parent, const std::vector<NodePtr>& children, Flags flags)
            : parent(parent), children(std::make_unique<std::vector<NodePtr>>(children)), flags(std::move(flags)) {}

        /// Modified parent of this Node, if it was moved.
        valid_ptr<Node*> parent;

        /// Modified children of this Node, should they have been modified.
        std::unique_ptr<std::vector<NodePtr>> children;

        /// Modified flags of this Node.
        Flags flags;
    };
    using DataPtr = std::unique_ptr<Data>;

protected:
    /// Number of user-definable flags on this system.
    static constexpr size_t s_user_flag_count = bitset_size_v<Flags> - s_internal_flag_count;

    // methods --------------------------------------------------------------------------------- //
protected:
    /// Value constructor.
    /// @param parent   Parent of this Node.
    Node(valid_ptr<Node*> parent);

public:
    NOTF_NO_COPY_OR_ASSIGN(Node);

    /// Destructor.
    virtual ~Node();

    // identification ---------------------------------------------------------

    /// Uuid of this Node.
    Uuid get_uuid() const noexcept { return m_uuid; }

    /// The Graph-unique name of this Node.
    /// @returns    Name of this Node. Is an l-value because the name of the Node may change.
    std::string get_name() const { return TheGraph()->get_name(_get_handle()); }

    /// (Re-)Names the Node.
    /// If another Node with the same name already exists in the Graph, this method will append the lowest integer
    /// postfix that makes the name unique.
    /// @param name     Proposed name of the Node.
    /// @returns        Actual new name of the Node.
    std::string set_name(const std::string& name);

    // properties -------------------------------------------------------------

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

    /// The parent of this Node.
    NodeHandle get_parent() const;

    /// @{
    /// Tests, if this Node is a descendant of the given ancestor.
    /// @param ancestor         Potential ancestor to verify.
    bool has_ancestor(const Node* ancestor) const;
    bool has_ancestor(const NodeHandle& ancestor) const {
        if (const NodeConstPtr ancestor_lock = NodeHandle::AccessFor<Node>::get_node_ptr(ancestor)) {
            return has_ancestor(ancestor_lock.get());
        }
        return false;
    }
    /// @}

    /// Finds and returns the first common ancestor of two Nodes.
    /// At the latest, the RootNode is always a common ancestor.
    /// If the handle passed in is expired, the returned handle will also be expired.
    /// @param other            Other Node to find the common ancestor for.
    /// @throws HierarchyError  If there is no common ancestor.
    NodeHandle get_common_ancestor(const NodeHandle& other) const;

    /// Returns the first ancestor of this Node that has a specific type (can be empty if none is found).
    /// @returns    Typed handle of the first ancestor with the requested type, can be empty if none was found.
    template<class T, typename = std::enable_if_t<std::is_base_of<Node, T>::value>>
    NodeHandle get_first_ancestor() const {
        NOTF_ASSERT(this_thread::is_the_ui_thread()); // method is const, but not thread-safe
        Node* next = _get_parent();
        if (auto* result = dynamic_cast<T*>(next)) { return NodeHandle(result->shared_from_this()); }
        while (next != next->_get_parent()) {
            next = next->_get_parent();
            if (auto* result = dynamic_cast<T*>(next)) { return NodeHandle(result->shared_from_this()); }
        }
        return {};
    }

    /// The number of direct children of this Node.
    size_t get_child_count() const {
        NOTF_ASSERT(this_thread::is_the_ui_thread()); // method is const, but not thread-safe
        return _read_children().size();
    }

    /// Returns a handle to a child Node at the given index.
    /// Index 0 is the node furthest back, index `size() - 1` is the child drawn at the front.
    /// @param index    Index of the Node.
    /// @returns        The requested child Node.
    /// @throws OutOfBounds    If the index is out-of-bounds or the child Node is of the wrong type.
    NodeHandle get_child(size_t index) const;

    /// Destroys this Node by deleting the owning pointer in its parent.
    /// This method is basically a destructor, make sure not to dereference this Node after this function call!
    void remove() {
        NOTF_ASSERT(this_thread::is_the_ui_thread());
        NodePtr parent;
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
        parent->_remove_child(shared_from_this());
    }
    // z-order ----------------------------------------------------------------

    /// Checks if this Node is in front of all of its siblings.
    bool is_in_front() const;

    /// Checks if this Node is behind all of its siblings.
    bool is_in_back() const;

    /// Returns true if this node is stacked anywhere in front of the given sibling.
    /// Also returns false if the handle is expired, the given Node is not a sibling or it is the same as this.
    /// @param sibling  Sibling node to test against.
    bool is_before(const NodeHandle& sibling) const;

    /// Returns true if this node is stacked anywhere behind the given sibling.
    /// Also returns false if the handle is expired, the given Node is not a sibling or it is the same as this.
    /// @param sibling  Sibling node to test against.
    bool is_behind(const NodeHandle& sibling) const;

    /// Moves this Node in front of all of its siblings.
    void stack_front();

    /// Moves this Node behind all of its siblings.
    void stack_back();

    /// Moves this Node before a given sibling.
    /// @param sibling  Sibling to stack before.
    /// @throws hierarchy_error If the sibling is not a sibling of this node.
    void stack_before(const NodeHandle& sibling);

    /// Moves this Node behind a given sibling.
    /// @param sibling  Sibling to stack behind.
    /// @throws hierarchy_error If the sibling is not a sibling of this node.
    void stack_behind(const NodeHandle& sibling);

protected: // for all subclasses
    // properties -------------------------------------------------------------

    /// (Re-)Defines a callback to be invoked every time the value of the Property is about to change.
    /// If the callback returns false, the update is cancelled and the old value remains.
    /// If the callback returns true, the update will proceed.
    /// Since the value is passed in by mutable reference, it can modify the value however it wants to. Even if the new
    /// value ends up the same as the old, the update will proceed. Note though, that the callback will only be called
    /// if the value is initially different from the one stored in the PropertyOperator.
    template<class T>
    void _set_property_callback(const std::string& property_name, typename Property<T>::callback_t callback) {
        NOTF_ASSERT(this_thread::is_the_ui_thread());
        _try_get_property<T>(property_name)->set_callback(std::move(callback));
    }

    // signals / slots --------------------------------------------------------

    /// Internal access to a Slot of this Node.
    /// Used to forward calls to the Slot from the outside to some callback inside the Node.
    /// @param name     Node-unique name of the Slot.
    /// @returns        The requested Slot.
    /// @throws         NameError / TypeError
    template<class T>
    typename Slot<T>::publisher_t _get_slot(const std::string& name) const {
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

    /// Tests a user defineable flag on this Node.
    /// @param index            Index of the user flag.
    /// @returns                True iff the flag is set.
    /// @throws OutOfBounds   If index >= user flag count.
    bool _get_flag(size_t index) const;

    /// Sets or unsets a user flag.
    /// @param index            Index of the user flag.
    /// @param value            Whether to set or to unser the flag.
    /// @throws OutOfBounds   If index >= user flag count.
    void _set_flag(size_t index, bool value = true);

    // hierarchy --------------------------------------------------------------

    /// Creates and adds a new child to this node.
    /// @param parent   Parent of the Node, must be `this` (is used for type checking).
    /// @param args     Arguments that are forwarded to the constructor of the child.
    /// @throws InternalError   If you pass anything else but `this` as the parent.
    template<class Child, class Parent, class... Args
#ifndef NOTF_TEST
             ,
             class = std::enable_if_t<detail::can_node_parent<Parent, Child>()>>
#else
             >
#endif
    detail::NewNode<Child> _create_child(Parent* parent, Args&&... args) {
        NOTF_ASSERT(this_thread::is_the_ui_thread());
        if (NOTF_UNLIKELY(parent != this)) {
            NOTF_THROW(InternalError, "Node::_create_child cannot be used to create children of other Nodes.");
        }

        auto child = std::make_shared<Child>(parent, std::forward<Args>(args)...);

        { // register the new node with the graph and store it as child
            auto node = std::static_pointer_cast<Node>(child);
            node->_finalize();
            TheGraph::AccessFor<Node>::register_node(node);
            _write_children().emplace_back(std::move(node));
        }

        return child;
    }

    /// Removes a child from this node.
    /// @param handle   Handle of the node to remove.
    void _remove_child(NodeHandle child_handle);

    /// Remove all children from this Node.
    void _clear_children();

    /// Changes the parent of this Node by first adding it to the new parent and then removing it from its old one.
    /// @param new_parent_handle    New parent of this Node. If it is the same as the old, this method does nothing.
    void _set_parent(NodeHandle new_parent_handle);

    /// Tests if the parent of
    template<class T>
    bool _has_parent_of_type() const {
        return dynamic_cast<T*>(_get_parent()) != nullptr;
    }

protected: // for direct subclasses only
    /// Reactive function marking this Node as dirty whenever a visible Property changes its value.
    std::shared_ptr<PropertyObserver>& _get_property_observer() { return m_property_observer; }

    /// Whether or not this Node has been finalized or not.
    bool _is_finalized() const { return _get_internal_flag(to_number(InternalFlags::FINALIZED)); }

private:
    /// Implementation specific query of a Property, returns an empty pointer if no Property by the given name is found.
    /// @param name     Node-unique name of the Property.
    virtual AnyPropertyPtr _get_property_impl(const std::string& /*name*/) const = 0;

    /// Implementation specific query of a Slot, returns an empty pointer if no Slot by the given name is found.
    /// @param name     Node-unique name of the Slot.
    virtual AnySlot* _get_slot_impl(const std::string& /*name*/) const = 0;

    /// Implementation specific query of a Signal, returns an empty pointer if no Signal by the given name is found.
    /// @param name     Node-unique name of the Signal.
    virtual AnySignalPtr _get_signal_impl(const std::string& /*name*/) const = 0;

    /// Calculates the combined hash value of all Properties.
    virtual size_t _calculate_property_hash(size_t result = detail::version_hash()) const = 0;

    /// Removes all modified data from all Properties.
    virtual void _clear_modified_properties() = 0;

    /// Allows const methods to create NodeHandles to this node.
    NodeHandle _get_handle() const { return NodeHandle(const_cast<Node*>(this)->shared_from_this()); }

    /// Access to the parent of this Node.
    /// Never creates a modified copy.
    Node* _get_parent() const;

    /// Finalizes this Node.
    /// Called on every new Node instance right after the Constructor of the most derived class has finished.
    /// Therefore, we do no have to ensure that the Graph is frozen etc.
    void _finalize() {
        // do not check whether this is the UI thread as we need this method during Application construction
        m_flags[to_number(InternalFlags::FINALIZED)] = true;
    }

    /// Deletes all modified data of this Node.
    void _clear_modified_data();

    /// Run time access to a Property of this Node.
    /// @param name     Node-unique name of the Property.
    /// @returns        Handle to the requested Property.
    /// @throws         NameError / TypeError
    template<class T>
    PropertyPtr<T> _try_get_property(const std::string& name) const {
        // can be accessed from both the UI and the render thread
        AnyPropertyPtr property = _get_property_impl(name);
        if (!property) { NOTF_THROW(NameError, "Node \"{}\" has no Property called \"{}\"", get_name(), name); }

        PropertyPtr<T> typed_property = std::dynamic_pointer_cast<Property<T>>(std::move(property));
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
    Slot<T>* _try_get_slot(const std::string& name) const {
        AnySlot* any_slot = _get_slot_impl(name);
        if (!any_slot) { NOTF_THROW(NameError, "Node \"{}\" has no Slot called \"{}\"", get_name(), name); }

        Slot<T>* slot = dynamic_cast<Slot<T>*>(any_slot);
        if (!slot) {
            NOTF_THROW(TypeError,
                       "Slot \"{}\" of Node \"{}\" is of type \"{}\", but was requested as \"{}\"", //
                       name, get_name(), any_slot->get_type_name(), type_name<T>());
        }
        return slot;
    }

    /// Run time access to a Signal of this Node.
    /// @param name     Node-unique name of the Signal.
    /// @returns        The requested Signal.
    /// @throws         NameError / TypeError
    template<class T>
    SignalPtr<T> _try_get_signal(const std::string& name) const {
        AnySignalPtr any_signal = _get_signal_impl(name);
        if (!any_signal) { NOTF_THROW(NameError, "Node \"{}\" has no Signal called \"{}\"", get_name(), name); }

        SignalPtr<T> signal = std::dynamic_pointer_cast<Signal<T>>(any_signal);
        if (!signal) {
            NOTF_THROW(TypeError,
                       "Signal \"{}\" of Node \"{}\" is of type \"{}\", but was requested as \"{}\"", //
                       name, get_name(), any_signal->get_type_name(), type_name<T>());
        }
        return signal;
    }

    /// Finds and returns the first common ancestor of two Nodes.
    /// At the latest, the RootNode is always a common ancestor.
    /// @param other            Other Node to find the common ancestor for.
    /// @throws HierarchyError  If there is no common ancestor.
    const Node* _get_common_ancestor(const Node* other) const;

    /// All children of this node, orded from back to front.
    /// Never creates a modified copy.
    const std::vector<NodePtr>& _read_children() const;

    /// All children of this node, orded from back to front.
    /// Will create a modified copy the current list of children if there is no copy yet and the Graph is frozen.
    std::vector<NodePtr>& _write_children();

    /// All children of the parent.
    const std::vector<NodePtr>& _read_siblings() const;

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
    void _mark_as_dirty();

    // fields ---------------------------------------------------------------------------------- //
private:
    /// Uuid of this Node.
    const Uuid m_uuid = Uuid::generate();

    /// Parent of this Node.
    Node* m_parent;

    /// All children of this Node, ordered from back to front (later Nodes are drawn on top of earlier ones).
    std::vector<NodePtr> m_children;

    /// Additional flags, contains both internal and user-definable flags.
    Flags m_flags;

    /// Pointer to modified Data, should this Node have been modified while the Graph has been frozen.
    DataPtr m_modified_data;

    /// Hash of all Property values of this Node.
    size_t m_property_hash;

    /// Combines the Property hash with the Node hashes of all children in order.
    size_t m_node_hash = 0;

    /// Reactive function marking this Node as dirty whenever a visible Property changes its value.
    PropertyObserverPtr m_property_observer = std::make_shared<PropertyObserver>(*this);
};

// node accessors =================================================================================================== //

template<>
class Accessor<Node, RootNode> {
    friend RootNode;

    /// Finalizes the given RootNode.
    static void finalize(Node& node) { node._finalize(); }

    /// Direct write access to child Nodes.
    static std::vector<NodePtr>& write_children(Node& node) { return node._write_children(); }
};

template<>
class Accessor<Node, Window> {
    friend Window;

    /// Finalizes the given Window(Node).
    static void finalize(Node& node) { node._finalize(); }
};

template<>
class Accessor<Node, detail::Graph> {
    friend detail::Graph;

    /// Deletes all modified data of this Node.
    static void clear_modified_data(Node& node) { node._clear_modified_data(); }
};

NOTF_CLOSE_NAMESPACE