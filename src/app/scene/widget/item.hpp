#pragma once

#include "app/ids.hpp"
#include "common/exception.hpp"
#include "common/id.hpp"
#include "common/signal.hpp"

NOTF_OPEN_NAMESPACE

//====================================================================================================================//

/// Exception type for errors originating in the Item hierarchy.
NOTF_EXCEPTION_TYPE(item_hierarchy_error)

//====================================================================================================================//

/// An Item is the base class for all objects in the `Item hierarchy`.
/// Its three main specializations are `Widgets`, `Layouts` and `Controllers`.
///
/// Lifetime
/// ========
///
/// The lifetime of Items is managed through a shared_ptr. This way we can have, for example, the same controller in
/// different places in the Item hierarchy.
///
/// Item Hierarchy
/// ==============
/// Starting with the RootLayout at the root, which is owned by a Window, every Item is owned by its immediate parent
/// Item through a shared pointer.
///
/// Item IDs
/// ========
/// Each Item has a constant unique integer ID assigned to it upon instantiation.
/// It can be used to identify the Item in a map, for debugging purposes or in conditionals.
///
/// Item name
/// =========
/// In addition to the unique ID, each Item can have a name.
/// The name is assigned by the user and is not guaranteed to be unique.
/// If the name is not set, it is simply the ID of the Item.
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
        template<bool is_const>
        class _Iterator {

            // types ----------------------------------------------------------
        private:
            /// (const) ChildContainer
            using Container = std::conditional_t<is_const, const ChildContainer, ChildContainer>;

            /// (const) Item
            using Child = std::conditional_t<is_const, const Item, Item>;

            // methods --------------------------------------------------------
        public:
            /// Constructor.
            /// @param container    Container to iterate.
            /// @param index        Start index to iterate from.
            _Iterator(Container& container, const size_t index) : container(container), index(index) {}

            /// Preincrement operator.
            _Iterator& operator++()
            {
                ++index;
                return *this;
            }

            /// Equality operator.
            bool operator==(const _Iterator& other) const { return index == other.index; }

            /// Inequality operator.
            bool operator!=(const _Iterator& other) const { return !(*this == other); }

            /// Dereferencing operator.
            Child& operator*() const { return *container.child(index); }

            // fields ---------------------------------------------------------
        public:
            /// Container to iterate.
            Container& container;

            /// Iteration index.
            size_t index;
        };

    public:
        using Iterator      = _Iterator</* is_const = */ false>;
        using ConstIterator = _Iterator</* is_const = */ true>;

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
        virtual Item* child(const size_t index) = 0;
        const Item* child(const size_t index) const { return const_cast<ChildContainer*>(this)->child(index); }
        /// }@

        /// {@
        /// Iterator to the first child of this Container.
        Iterator begin() { return Iterator(*this, 0); }
        ConstIterator begin() const { return ConstIterator(*this, 0); }
        /// }@

        /// {@
        /// Iterator one past the last child of this Container.
        Iterator end() { return Iterator(*this, size()); }
        ConstIterator end() const { return ConstIterator(*this, size()); }
        /// }@

        /// Disconnects all child Items from their parent.
        /// Is virtual so that subclasses can do additional operations (like clearing an underlying vector etc.)
        virtual void clear()
        {
            for (Item& item : *this) {
                item._set_parent(nullptr, /* is_orphaned = */ false);
            }
        }

        /// Checks whether this Container contains a given Item.
        bool contains(const Item* candidate) const
        {
            for (const Item& item : *this) {
                if (&item == candidate) {
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
            for (Item& item : *this) {
                item._set_parent(nullptr, /* is_orphaned = */ true);
            }
        }
    };

    /// Unique pointer to child item container.
    using ChildContainerPtr = std::unique_ptr<ChildContainer>;

    // signals -------------------------------------------------------------------------------------------------------//
public:
    /// Emitted when this Item got a new parent.
    /// @param The new parent.
    Signal<Item&> on_parent_changed;

    // methods -------------------------------------------------------------------------------------------------------//
protected:
    /// Constructor.
    Item(ChildContainerPtr container);

    // methods -------------------------------------------------------------------------------------------------------//
public:
    NOTF_NO_COPY_OR_ASSIGN(Item)

    /// Destructor
    virtual ~Item();

    /// Application-unique ID of this Item.
    ItemID id() const { return m_id; }

    /// The name of this Item.
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
    /// Returns the closest Layout in the hierarchy of the given Item.
    /// Is empty if the given Item has no ancestor Layout.
    risky_ptr<Layout> layout();
    risky_ptr<const Layout> layout() const { return const_cast<Item*>(this)->layout(); }
    ///@}

    ///@{
    /// Returns the closest Controller in the hierarchy of the given Item.
    /// Is empty if the given Item has no ancestor Controller.
    risky_ptr<Controller> controller();
    risky_ptr<const Controller> controller() const { return const_cast<Item*>(this)->controller(); }
    ///@}

    ///@{
    /// Returns the ScreenItem associated with this given Item - either the Item itself or a Controller's root Item.
    /// Is empty if this is a Controller without a root Item.
    risky_ptr<ScreenItem> screen_item();
    risky_ptr<const ScreenItem> screen_item() const { return const_cast<Item*>(this)->screen_item(); }
    ///@}

    /// Updates the name of this Item.
    const std::string& set_name(std::string name)
    {
        m_name = std::move(name);
        return m_name;
    }

protected:
    /// Removes a child Item from this Item.
    /// This needs to be a virtual method, because Items react differently to the removal of a child Item.
    virtual void _remove_child(const Item* child_item) = 0;

    /// Pulls new values from the parent if it changed.
    virtual void _update_from_parent() {}

    /// Returns the first ancestor of this Item that has a specific type (can be empty if none is found).
    template<typename Type>
    Type* _first_ancestor() const
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

    /// Allows Item subclasses to set each others' parent.
    static void _set_parent(Item* item, Item* parent) { item->_set_parent(parent, /* is_orphaned = */ false); }

private:
    /// Sets the parent of this Item.
    /// @param is_orphaned   If the parent of the Item has already been deleted, the Item cannot unregister itself.
    void _set_parent(Item* parent, bool is_orphaned);

    // fields --------------------------------------------------------------------------------------------------------//
protected:
    /// All children of this Item.
    ChildContainerPtr m_children;

private:
    /// Application-unique ID of this Item.
    const ItemID m_id;

    /// The parent Item, is guaranteed to be valid iff `m_window` is valid.
    Item* m_parent;

    /// An optional name of this Item.
    /// The name is set by the user and is not guaranteed to be unique.
    /// If the name is not set, it is simply the ID of the Item.
    std::string m_name;
};

//====================================================================================================================//

namespace detail {
/// Widgets have no child Items and use this empty Container as a placeholder instead.
struct EmptyItemContainer final : public Item::ChildContainer {
    virtual ~EmptyItemContainer() override;

    virtual size_t size() const override { return 0; }

    virtual Item* child(const size_t) override
    {
        notf_throw(out_of_bounds, "Child Item with an out-of-bounds index requested");
    }
};

//====================================================================================================================//

/// Controller (and some Layouts) have a single child Item.
struct SingleItemContainer final : public Item::ChildContainer {
    virtual ~SingleItemContainer() override;

    virtual size_t size() const override { return (item ? 1 : 0); }

    virtual Item* child(const size_t index) override
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

    virtual Item* child(const size_t index) override
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
