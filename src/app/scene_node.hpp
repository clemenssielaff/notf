#pragma once

#include "app/ids.hpp"
#include "app/property_graph.hpp"
#include "common/signal.hpp"

NOTF_OPEN_NAMESPACE

//====================================================================================================================//
// TODO: update documentation for SceneNode
/// A ScreenNode is the virtual base class for all objects in the Item hierarchy. Its three main specializations are
/// `Widgets`, `Layouts` and `Controllers`.
///
/// Lifetime
/// ========
///
/// The lifetime of Items is managed through a shared_ptr. This way, the user is free to keep a subhierarchy around
/// even after its parent has gone out of scope.
///
/// Hierarchy
/// =========
///
/// Items form a hierarchy with a single root Item on top. The number of children that an Item can have depends on its
/// type. Widgets have no children, Controller have a single Layout as a child and Layouts can have a (theoretically
/// unlimited) number of children of all types.
///
/// Since Layouts may have special container requirement for their children (a list, a map, a matrix ...), Items have a
/// virtual container class called `ChildContainer` that allows high level access to the children of each Item,
/// regardless of how they are stored in memory. The only requirements that a Container must fulfill is that it has to
/// offer a `size()` function that returns the number of children in the layout, a `clear()` function that removes all
/// children (thereby potentially destroying them) and the method `child_at(index)` which returns a mutable reference to
/// an pointer of the child at the requested index, or throws an `out_of_bounds` exception if the index is >= the
/// Container's size.
///
/// SceneNodes keep a raw pointer to their parent. The alternative would be to have a weak_ptr to the parent and lock
/// it, whenever we need to go up in the hierarchy. However, going up in the hierarchy is a very common occurrence and
/// with deeply nested layouts, I assume that the number of locking operations per second will likely go in the
/// thousands. This is a non-neglible expense for something that can prevented by making sure that you either know that
/// the parent is still alive, or you check first. The pointer is initialized to zero and parents let their children
/// know when they are destroyed. While this still leaves open the possiblity of an Item subclass manually messing up
/// the parent pointer using the static `_set_parent` method, I have to stop somewhere and trust the user not to break
/// things.
///
/// ID
/// ==
///
/// Each SceneNode has a constant unique integer ID assigned to it upon instantiation. It can be used to identify the
/// node in a map, for debugging purposes or in conditionals.
///
/// Name
/// ====
///
/// In addition to the unique ID, each SceneNode can have a name. The name is assigned by the user and is not guaranteed
/// to be unique. If the name is not set, it is custom to log the SceneNode id instead, formatted like this:
///
///     log_info << "Something cool happened to SceneNode #" << node.id() << ".";
///
/// Signals
/// =======
///
/// SceneNodes communicate with each other either through their relationship in the hierachy (parents to children and
/// vice-versa) or via Signals. Signals have the advantage of being able to connect every SceneNode regardless of its
/// position in the hierarchy. They can be created by the user and enabled/disabled at will.
/// In order to facilitate Signal handling at the lowest possible level, all SceneNodes derive from the
/// `receive_signals` class that takes care of removing leftover connections that still exist once the SceneNode goes
/// out of scope.
///
class SceneNode : public receive_signals, public std::enable_shared_from_this<SceneNode> {

    // types ---------------------------------------------------------------------------------------------------------//
public:
    /// Abstract child SceneNode Container.
    /// Is used by subclasses to abstract away how (and if) they store child SceneNodes.
    class ChildContainer {
        friend class notf::SceneNode;

        // types --------------------------------------------------------------
    private:
        /// All ChildContainers must be able to sort their children into a flat list ordered from front to back.
        /// That means that the first SceneNode will be drawn on top of the second and so on.
        template<bool IS_CONST, bool IS_REVERSE>
        class _Iterator {

            // types ----------------------------------------------------------
        private:
            /// (const) ChildContainer
            using Container = std::conditional_t<IS_CONST, const ChildContainer, ChildContainer>;

            /// (const) SceneNode
            using ChildPtr = std::conditional_t<IS_CONST, const SceneNode*, SceneNode*>;

        public:
            /// Index type
            using index_t = size_t;

            // methods --------------------------------------------------------
        public:
            /// Constructor.
            /// @param container    Container to iterate.
            /// @param index        Start index to iterate from.
            _Iterator(Container& container, const index_t index) : container(container), index(index) {}

            /// Forward preincrement operator.
            template<bool is_reverse = IS_REVERSE>
            typename std::enable_if_t<!is_reverse, _Iterator&> operator++()
            {
                ++index;
                return *this;
            }

            /// Reverse preincrement operator.
            template<bool is_reverse = IS_REVERSE>
            typename std::enable_if_t<is_reverse, _Iterator&> operator++()
            {
                --index;
                return *this;
            }

            /// Equality operator.
            bool operator==(const _Iterator& other) const { return index == other.index; }

            /// Inequality operator.
            bool operator!=(const _Iterator& other) const { return !(*this == other); }

            /// Dereferencing operator.
            ChildPtr operator*() const { return container.child_at(index); }

            // fields ---------------------------------------------------------
        public:
            /// Container to iterate.
            Container& container;

            /// Iteration index.
            index_t index;
        };

    public:
        using Iterator = _Iterator</* IS_CONST = */ false, /* IS_REVERSE = */ false>;
        using ConstIterator = _Iterator</* IS_CONST = */ true, /* IS_REVERSE = */ false>;
        using ReverseIterator = _Iterator</* IS_CONST = */ false, /* IS_REVERSE = */ true>;
        using ReverseConstIterator = _Iterator</* IS_CONST = */ true, /* IS_REVERSE = */ true>;

        // methods ------------------------------------------------------------
    public:
        /// Destructor.
        virtual ~ChildContainer();

        /// Number of children in the Container.
        virtual size_t size() const = 0;

        /// {@
        /// Returns a child SceneNode by its index.
        /// @param index            Index of the requested child.
        /// @throws out_of_range    If the index is >= the size of this Container.
        virtual SceneNode* child_at(const size_t index) = 0;
        const SceneNode* child_at(const size_t index) const
        {
            return const_cast<ChildContainer*>(this)->child_at(index);
        }
        /// }@

        /// {@
        /// Iterator to the first child of this Container.
        Iterator begin() { return Iterator(*this, 0); }
        ConstIterator begin() const { return ConstIterator(*this, 0); }
        /// }@

        /// {@
        /// Reverse Iterator to the last child of this Container.
        ReverseIterator rbegin() { return ReverseIterator(*this, _last_index()); }
        ReverseConstIterator rbegin() const { return ReverseConstIterator(*this, _last_index()); }
        /// }@

        /// {@
        /// Iterator one past the last child of this Container.
        Iterator end() { return Iterator(*this, size()); }
        ConstIterator end() const { return ConstIterator(*this, size()); }
        /// }@

        /// {@
        /// Iterator one past the last child of this Container.
        ReverseIterator rend() { return ReverseIterator(*this, std::numeric_limits<ReverseIterator::index_t>::max()); }
        ReverseConstIterator rend() const
        {
            return ReverseConstIterator(*this, std::numeric_limits<ReverseConstIterator::index_t>::max());
        }
        /// }@

        /// Disconnects all child SceneNodes from their parent.
        /// Is virtual so that subclasses can do additional operations (like clearing an underlying vector etc.)
        virtual void clear()
        {
            for (SceneNode* node : *this) {
                node->_set_parent(nullptr, /* notify_old = */ true);
            }
        }

        /// Checks whether this Container contains a given SceneNode.
        bool contains(const SceneNode* candidate) const
        {
            for (const SceneNode* node : *this) {
                if (node == candidate) {
                    return true;
                }
            }
            return false;
        }

    private:
        /// Sets the parent of all SceneNodes to nullptr without evoking proper reparenting.
        /// Is only used by the SceneNode destructor.
        void _destroy()
        {
            for (SceneNode* node : *this) {
                node->_set_parent(nullptr, /* notify_old = */ false);
            }
        }

        /// Returns the index of the last SceneNode in this Container.
        size_t _last_index() const
        {
            const auto my_size = size();
            return my_size == 0 ? 0 : my_size;
        }
    };

    /// Unique pointer to child SceneNode container.
    using ChildContainerPtr = std::unique_ptr<ChildContainer>;

protected:
    /// Token object to make sure that object instances can only be created by a call to `_create`.
    class Token {
        friend class SceneNode;
        Token() {} // not "= default", otherwise you could construct a Token via `Token{}`.
    };

    // signals -------------------------------------------------------------------------------------------------------//
public:
    /// Emitted when this SceneNode got a new parent.
    /// @param The new parent.
    Signal<SceneNode*> on_parent_changed;

    /// Emitted when this SceneNode changes its name.
    /// @param The new name.
    Signal<const std::string&> on_name_changed;

    // methods -------------------------------------------------------------------------------------------------------//
protected:
    /// Constructor.
    /// @param token        Factory token provided by SceneNode::_create.
    /// @param container    Container used to store this SceneNode's children.
    SceneNode(const Token&, ChildContainerPtr container);

protected:
    /// Factory method for this type and all of its children.
    /// You need to call this function from your own factory in order to get a Token instance.
    /// This method will in turn register the new instance with the SceneManager.
    template<typename T, typename... Ts>
    static std::shared_ptr<T> _create(Ts&&... args)
    {
        static_assert(std::is_base_of<SceneNode, T>::value, "SceneNode::_create can only create instances of SceneNode "
                                                            "subclasses");
        const Token token;
        return NOTF_MAKE_SHARED_FROM_PRIVATE(T, token, std::forward<Ts>(args)...);
    }

public:
    /// Destructor
    virtual ~SceneNode();

    /// Application-unique ID of this SceneNode.
    SceneNodeId id() const { return m_id; }

    /// The user-defined name of this SceneNode, is potentially empty and not guaranteed to be unique if set.
    const std::string& name() const { return m_name; }

    ///@{
    /// The children of this SceneNode.
    const ChildContainer& children() const { return *m_children; }
    ChildContainer& children() { return *m_children; }
    ///@}

    /// Checks if this SceneNode currently has a parent or not.
    bool has_parent() const { return m_parent != nullptr; }

    ///@{
    /// The parent of this SceneNode, returns an empty pointer if this SceneNode currently has no parent.
    risky_ptr<SceneNode> parent() { return m_parent; }
    risky_ptr<const SceneNode> parent() const { return const_cast<SceneNode*>(this)->parent(); }
    ///@}

    /// Tests, if this SceneNode is a descendant of the given ancestor.
    bool has_ancestor(const SceneNode* ancestor) const;

    ///@{
    /// Finds and returns the first common ancestor of two SceneNodes, returns empty if none exists.
    risky_ptr<SceneNode> common_ancestor(SceneNode* other);
    risky_ptr<const SceneNode> common_ancestor(const SceneNode* other) const
    {
        return const_cast<SceneNode*>(this)->common_ancestor(const_cast<SceneNode*>(other));
    }
    ///@}

    ///@{
    /// Returns the first ancestor of this SceneNode that has a specific type (can be empty if none is found).
    template<typename Type>
    risky_ptr<Type> first_ancestor()
    {
        SceneNode* next = m_parent;
        while (next) {
            if (Type* result = dynamic_cast<Type*>(next)) {
                return result;
            }
            next = next->m_parent;
        }
        return {};
    }
    template<typename Type>
    risky_ptr<const Type> first_ancestor() const
    {
        return const_cast<SceneNode*>(this)->first_ancestor<Type>();
    }
    ///@}

    /// Updates the name of this SceneNode.
    /// @param name New name of this SceneNode.
    const std::string& set_name(std::string name);

protected:
    /// Removes a child from this SceneNode.
    /// This needs to be a virtual method, because SceneNodes react differently to the removal of a child.
    virtual void _remove_child(const SceneNode* child_node) = 0;

    /// Queries new data from the parent (what that is depends on the SceneNode type).
    virtual void _update_from_parent() {}

    /// Allows SceneNode subclasses to set each others' parent.
    static void _set_parent(SceneNode* node, SceneNode* parent) { node->_set_parent(parent, /* notify_old = */ true); }

private:
    /// Sets the parent of this SceneNode.
    /// @param notify_old   If the old parent is already in the process of being deleted, the child SceneNode must not
    ///                     attempt to unregister itself anymore.
    void _set_parent(SceneNode* parent, bool notify_old);

    // fields --------------------------------------------------------------------------------------------------------//
protected:
    /// All children of this SceneNode.
    ChildContainerPtr m_children;

private:
    /// Non-owning pointer to the parent SceneNode, is potentially empty.
    SceneNode* m_parent = nullptr;

    /// The user-defined name of this SceneNode, is potentially empty and not guaranteed to be unique if set.
    std::string m_name;

    /// Application-unique ID of this Node.
    const SceneNodeId m_id;
};

//====================================================================================================================//

namespace detail {

/// Widgets have no child SceneNodes and use this empty Container as a placeholder instead.
struct EmptyNodeContainer final : public SceneNode::ChildContainer {
    virtual ~EmptyNodeContainer() override;

    virtual size_t size() const override { return 0; }

    virtual SceneNode* child_at(const size_t) override
    {
        notf_throw(out_of_bounds, "Child SceneNode with an out-of-bounds index requested");
    }
};

//====================================================================================================================//

/// Controller (and some Layouts) have a single child SceneNode.
struct SingleNodeContainer final : public SceneNode::ChildContainer {
    virtual ~SingleNodeContainer() override;

    virtual size_t size() const override { return (node ? 1 : 0); }

    virtual SceneNode* child_at(const size_t index) override
    {
        if (index != 0) {
            notf_throw(out_of_bounds, "Child SceneNode with an out-of-bounds index requested");
        }
        return node.get();
    }

    virtual void clear() override
    {
        SceneNode::ChildContainer::clear();
        node.reset();
    }

    /// The singular SceneNode contained in this Container.
    SceneNodePtr node;
};

//====================================================================================================================//

/// Many Layouts keep their child SceneNodes in a list.
struct NodeList final : public SceneNode::ChildContainer {
    virtual ~NodeList() override;

    virtual size_t size() const override { return nodes.size(); }

    virtual SceneNode* child_at(const size_t index) override
    {
        if (index >= nodes.size()) {
            notf_throw(out_of_bounds, "Child SceneNode with an out-of-bounds index requested");
        }
        return nodes[index].get();
    }

    virtual void clear() override
    {
        SceneNode::ChildContainer::clear();
        nodes.clear();
    }

    /// All SceneNodes contained in the list.
    std::vector<SceneNodePtr> nodes;
};

} // namespace detail

NOTF_CLOSE_NAMESPACE
