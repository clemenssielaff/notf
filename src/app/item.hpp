#pragma once

#include "app/ids.hpp"
#include "app/property_graph.hpp"
#include "common/signal.hpp"
#include "utils/make_smart_enabler.hpp"

NOTF_OPEN_NAMESPACE

//====================================================================================================================//

/// An Item is the virtual base class for all objects in the Item hierarchy. Its three main specializations are
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
/// an ItemPtr of the child at the requested index, or throws an `out_of_bounds` exception if the index is >= the
/// Container's size.
///
/// Items keep a raw pointer to their parent. The alternative would be to have a weak_ptr to the parent and lock it,
/// whenever we need to go up in the hierarchy. However, going up in the hierarchy is a very common occurrence and with
/// deeply nested layouts, I assume that the number of locking operations per second will likely go in the thousands.
/// This is a non-neglible expense for something that can prevented by making sure that you either know that the parent
/// is still alive, or you check first.
/// The pointer is initialized to zero and parents let their children know when they are destroyed. While this still
/// leaves open the possiblity of an Item subclass manually messing up the parent pointer using the static `_set_parent`
/// method, I have to stop somewhere and trust the user not to break things.
///
/// ID
/// ==
///
/// Each Item has a constant unique integer ID assigned to it upon instantiation. It can be used to identify the Item in
/// a map, for debugging purposes or in conditionals.
///
/// Name
/// ====
///
/// In addition to the unique ID, each Item can have a name. The name is assigned by the user and is not guaranteed to
/// be unique. If the name is not set, it is custom to log the Item id instead, formatted like this:
///
///     log_info << "Something cool happened to Item #" << item.id() << ".";
///
/// Signals
/// =======
///
/// Items communicate with each other either through their relationship in the hierachy (parents to children and
/// vice-versa) or via Signals. Signals have the advantage of being able to connect every Item regardless of its
/// position in the hierarchy. They can be created by the user and enabled/disabled at will.
/// In order to facilitate Signal handling at the lowest possible level, all Items derive from the `receive_signals`
/// class that takes care of removing leftover connections that still exist once the Item goes out of scope.
///
class Item : public receive_signals, public std::enable_shared_from_this<Item> {

    // types ---------------------------------------------------------------------------------------------------------//
public:
    /// Abstract child Item Container.
    /// Is used by subclasses to abstract away how (and if) they store child Items.
    class ChildContainer {
        friend class notf::Item;

        // types --------------------------------------------------------------
    private:
        /// All ChildContainers must be able to sort their children into a flat list ordered from front to back.
        /// That means that the first Item will be drawn on top of the second and so on.
        template<bool IS_CONST, bool IS_REVERSE>
        class _Iterator {

            // types ----------------------------------------------------------
        private:
            /// (const) ChildContainer
            using Container = std::conditional_t<IS_CONST, const ChildContainer, ChildContainer>;

            /// (const) Item
            using ChildPtr = std::conditional_t<IS_CONST, const Item*, Item*>;

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
        using Iterator = _Iterator</* is_const = */ false, /* is_reverse = */ false>;
        using ConstIterator = _Iterator</* is_const = */ true, /* is_reverse = */ false>;
        using ReverseIterator = _Iterator</* is_const = */ false, /* is_reverse = */ true>;
        using ReverseConstIterator = _Iterator</* is_const = */ true, /* is_reverse = */ true>;

        // methods ------------------------------------------------------------
    public:
        /// Destructor.
        virtual ~ChildContainer();

        /// Number of children in the Container.
        virtual size_t size() const = 0;

        /// {@
        /// Returns a child Item by its index.
        /// @param index            Index of the requested child.
        /// @throws out_of_range    If the index is >= the size of this Container.
        virtual Item* child_at(const size_t index) = 0;
        const Item* child_at(const size_t index) const { return const_cast<ChildContainer*>(this)->child_at(index); }
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

        /// Disconnects all child Items from their parent.
        /// Is virtual so that subclasses can do additional operations (like clearing an underlying vector etc.)
        virtual void clear()
        {
            for (Item* item : *this) {
                item->_set_parent(nullptr, /* notify_old = */ true);
            }
        }

        /// Checks whether this Container contains a given Item.
        bool contains(const Item* candidate) const
        {
            for (const Item* item : *this) {
                if (item == candidate) {
                    return true;
                }
            }
            return false;
        }

    private:
        /// Sets the parent of all Items to nullptr without evoking proper reparenting.
        /// Is only used by the Item destructor.
        void _destroy()
        {
            for (Item* item : *this) {
                item->_set_parent(nullptr, /* notify_old = */ false);
            }
        }

        /// Returns the index of the last Item in this Container.
        size_t _last_index() const
        {
            const auto my_size = size();
            return my_size == 0 ? 0 : my_size;
        }
    };

    /// Unique pointer to child item container.
    using ChildContainerPtr = std::unique_ptr<ChildContainer>;

protected:
    /// Token object to make sure that object instances can only be created by a call to `_create`.
    class Token {
        friend class Item;
        Token() {} // not "= default", otherwise you could construct a Token via `Token{}`.
    };

    // signals -------------------------------------------------------------------------------------------------------//
public:
    /// Emitted when this Item got a new parent.
    /// @param The new parent.
    Signal<Item*> on_parent_changed;

    /// Emitted when this Item changes its name.
    /// @param The new name.
    Signal<const std::string&> on_name_changed;

    // methods -------------------------------------------------------------------------------------------------------//
protected:
    /// Constructor.
    /// @param token        Factory token provided by Item::_create.
    /// @param container    Container used to store this Item's children.
    Item(const Token&, ChildContainerPtr container);

    /// Factory method for this type and all of its children.
    /// You need to call this function from your own factory in order to get a Token instance.
    /// This method will in turn register the new instance with the SceneManager.
    template<typename T, typename... Ts>
    static std::shared_ptr<T> _create(Ts&&... args)
    {
        static_assert(std::is_base_of<Item, T>::value, "Item::_create can only create instances of Item subclasses");
        const Token token;
#ifdef NOTF_DEBUG
        auto result = std::shared_ptr<T>(new T(token, std::forward<Ts>(args)...));
#else
        auto result = std::make_shared<make_shared_enabler<T>>(token, std::forward<Ts>(args)...);
#endif
        return result;
    }

public:
    /// Destructor
    virtual ~Item();

    /// Application-unique ID of this Item.
    ItemId id() const { return m_id; }

    /// The user-defined name of this Item, is potentially empty and not guaranteed to be unique if set.
    const std::string& name() const { return m_name; }

    ///@{
    /// The children of this Item.
    const ChildContainer& children() const { return *m_children; }
    ChildContainer& children() { return *m_children; }
    ///@}

    /// Checks if this Item currently has a parent or not.
    bool has_parent() const { return m_parent != nullptr; }

    ///@{
    /// The parent of this Item, returns an empty pointer if this Item currently has no parent.
    risky_ptr<Item> parent() { return m_parent; }
    risky_ptr<const Item> parent() const { return const_cast<Item*>(this)->parent(); }
    ///@}

    /// Tests, if this Item is a descendant of the given ancestor Item.
    bool has_ancestor(const Item* ancestor) const;

    ///@{
    /// Finds and returns the first common ancestor of two Items, returns empty if none exists.
    risky_ptr<Item> common_ancestor(Item* other);
    risky_ptr<const Item> common_ancestor(const Item* other) const
    {
        return const_cast<Item*>(this)->common_ancestor(const_cast<Item*>(other));
    }
    ///@}

    ///@{
    /// Returns the first ancestor of this Item that has a specific type (can be empty if none is found).
    template<typename Type>
    risky_ptr<Type> first_ancestor()
    {
        Item* next = m_parent;
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
        return const_cast<Item*>(this)->first_ancestor<Type>();
    }
    ///@}

    /// Updates the name of this Item.
    /// @param name New name of this Item.
    const std::string& set_name(std::string name);

    // properties -------------------------------------------------------------

    /// Creates a new Property associated with this Item.
    /// @param value    Initial value of the Property, also used to determine its type.
    /// @returns        Property that can be used to access the Property value in the graph.
    //    template<typename T>
    //    Property<T> create_property(T&& value)
    //    {
    //        const PropertyKey key = PropertyKey(m_id, ++m_property_counter);
    //        PropertyGraph::Access<Item>(_property_graph()).add_property(key, std::forward<T>(value));
    //        return Property<T>(key);
    //    }

protected:
    /// Removes a child Item from this Item.
    /// This needs to be a virtual method, because Items react differently to the removal of a child Item.
    virtual void _remove_child(const Item* child_item) = 0;

    /// Queries new data from the parent (what that is depends on the Item type).
    virtual void _update_from_parent() {}

    /// Allows Item subclasses to set each others' parent.
    static void _set_parent(Item* item, Item* parent) { item->_set_parent(parent, /* notify_old = */ true); }

private:
    /// Sets the parent of this Item.
    /// @param notify_old   If the old parent is already in the process of being deleted, the child Item must not
    ///                     attempt to unregister itself anymore.
    void _set_parent(Item* parent, bool notify_old);

    // fields --------------------------------------------------------------------------------------------------------//
protected:
    /// All children of this Item.
    ChildContainerPtr m_children;

private:
    /// Non-owning pointer to the parent Item, is potentially empty.
    Item* m_parent = nullptr;

    /// The user-defined name of this Item, is potentially empty and not guaranteed to be unique if set.
    std::string m_name;

    /// Application-unique ID of this Item.
    const ItemId m_id;
};

//====================================================================================================================//

namespace detail {

/// Widgets have no child Items and use this empty Container as a placeholder instead.
struct EmptyItemContainer final : public Item::ChildContainer {
    virtual ~EmptyItemContainer() override;

    virtual size_t size() const override { return 0; }

    virtual Item* child_at(const size_t) override
    {
        notf_throw(out_of_bounds, "Child Item with an out-of-bounds index requested");
    }
};

//====================================================================================================================//

/// Controller (and some Layouts) have a single child Item.
struct SingleItemContainer final : public Item::ChildContainer {
    virtual ~SingleItemContainer() override;

    virtual size_t size() const override { return (item ? 1 : 0); }

    virtual Item* child_at(const size_t index) override
    {
        if (index != 0) {
            notf_throw(out_of_bounds, "Child Item with an out-of-bounds index requested");
        }
        return item.get();
    }

    virtual void clear() override
    {
        Item::ChildContainer::clear();
        item.reset();
    }

    /// The singular Item contained in this Container.
    ItemPtr item;
};

//====================================================================================================================//

/// Many Layouts keep their child Items in a list.
struct ItemList final : public Item::ChildContainer {
    virtual ~ItemList() override;

    virtual size_t size() const override { return items.size(); }

    virtual Item* child_at(const size_t index) override
    {
        if (index >= items.size()) {
            notf_throw(out_of_bounds, "Child Item with an out-of-bounds index requested");
        }
        return items[index].get();
    }

    virtual void clear() override
    {
        Item::ChildContainer::clear();
        items.clear();
    }

    /// All Items contained in the list.
    std::vector<ItemPtr> items;
};

} // namespace detail

NOTF_CLOSE_NAMESPACE
