#pragma once

#include "app/forwards.hpp"
#include "common/exception.hpp"
#include "common/id.hpp"
#include "common/signal.hpp"

namespace notf {

//====================================================================================================================//

/// Unique identification token of an Item.
using ItemID = IdType<Item, size_t>;

/// Exception type for errors originating in the Item hierarchy.
NOTF_EXCEPTION_TYPE(item_hierarchy_error)

//====================================================================================================================//

/// An Item is the base class for all objects in the `Item hierarchy`.
/// Its three main specializations are `Widgets`, `Layouts` and `Controllers`.
///
/// Item Hierarchy
/// ==============
/// Starting with the WindowLayout at the root, which is owned by a Window, every Item is owned by its immediate parent
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
    NOTF_ACCESS_TYPES(detail::ItemContainer)

    // signals -------------------------------------------------------------------------------------------------------//
public:
    /// Emitted when this Item got a new parent.
    /// @param The new parent.
    Signal<Item*> on_parent_changed;

    /// Emitted when this Item is moved to the Item hierarchy of a new Window.
    /// @param New Window.
    Signal<Window*> on_window_changed;

    // methods -------------------------------------------------------------------------------------------------------//
protected:
    /// Constructor.
    Item(detail::ItemContainerPtr container);

    // methods -------------------------------------------------------------------------------------------------------//
public:
    NO_COPY_AND_ASSIGN(Item)

    /// Destructor
    virtual ~Item();

    /// Application-unique ID of this Item.
    ItemID id() const { return m_id; }

    ///@{
    /// The parent of this Item, returns an empty pointer if this Item currently has no parent.
    risky_ptr<Item> parent() { return m_parent; }
    const risky_ptr<Item> parent() const { return const_cast<Item*>(this)->parent(); }
    ///@}

    /// Checks if this Item currently has a parent or not.
    bool has_parent() const { return m_parent != nullptr; }

    /// The name of this Item.
    const std::string& name() const { return m_name; }

    /// Checks if this Item is the parent of the given child.
    bool has_child(const Item* child) const;

    /// Checks if this Item has any children at all.
    bool has_children() const;

    /// Tests, if this Item is a descendant of the given ancestor Item.
    bool has_ancestor(const Item* ancestor) const;

    ///@{
    /// Finds and returns the first common ancestor of two Items, returns empty if none exists.
    risky_ptr<Item> common_ancestor(Item* other);
    const risky_ptr<Item> common_ancestor(const Item* other) const
    {
        return const_cast<Item*>(this)->common_ancestor(const_cast<Item*>(other));
    }
    ///@}

    ///@{
    /// Returns the closest Layout in the hierarchy of the given Item.
    /// Is empty if the given Item has no ancestor Layout.
    risky_ptr<Layout> layout();
    const risky_ptr<Layout> layout() const { return const_cast<Item*>(this)->layout(); }
    ///@}

    ///@{
    /// Returns the closest Controller in the hierarchy of the given Item.
    /// Is empty if the given Item has no ancestor Controller.
    risky_ptr<Controller> controller();
    const risky_ptr<Controller> controller() const { return const_cast<Item*>(this)->controller(); }
    ///@}

    ///@{
    /// Returns the ScreenItem associated with this given Item - either the Item itself or a Controller's root Item.
    /// Is empty if this is a Controller without a root Item.
    risky_ptr<ScreenItem> screen_item();
    const risky_ptr<ScreenItem> screen_item() const { return const_cast<Item*>(this)->screen_item(); }
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
    detail::ItemContainerPtr m_children;

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

/// Convenience function to create a correctly typed `shared_from_this` shared_ptr from Item subclasses.
template<typename ItemSubclass, ENABLE_IF_SUBCLASS(ItemSubclass, Item)>
std::shared_ptr<ItemSubclass> make_shared_from(ItemSubclass* item)
{
    return std::dynamic_pointer_cast<ItemSubclass>(static_cast<Item*>(item)->shared_from_this());
}

//====================================================================================================================//

namespace detail {

/// Abstract Item Container.
/// Used by Item subclasses to contain child Items.
struct ItemContainer {
    friend class notf::Item;

    /// Destructor.
    virtual ~ItemContainer();

    /// Clears all Items from this Container
    virtual void clear() = 0;

    /// Applies a function to all Items in this Container.
    virtual void apply(std::function<void(Item*)> function) = 0;

    /// Checks whether this Container contains a given Item.
    virtual bool contains(const Item* item) const = 0;

    /// Checks whether this Container is empty or not.
    virtual bool is_empty() const = 0;

    // methods -------------------------------------------------------------------------------------------------------//
protected:
    /// Sets the parent of all Items to nullptr without evoking proper reparenting.
    /// Is only used by the Item destructor.
    void destroy();
};

//====================================================================================================================//

/// Widgets have no child Items and use this empty Container as a placeholder instead.
struct EmptyItemContainer final : public ItemContainer {
    virtual ~EmptyItemContainer() override;

    virtual void clear() override {}

    virtual void apply(std::function<void(Item*)>) override {}

    virtual bool contains(const Item*) const override { return false; }

    virtual bool is_empty() const override { return true; }
};

//====================================================================================================================//

/// Controller (and some Layouts) have a single child Item.
struct SingleItemContainer final : public ItemContainer {
    virtual ~SingleItemContainer() override;

    virtual void clear() override;

    virtual void apply(std::function<void(Item*)> function) override;

    virtual bool contains(const Item* child) const override { return child == item.get(); }

    virtual bool is_empty() const override { return static_cast<bool>(item); }

    /// The singular Item contained in this Container.
    ItemPtr item;
};

//====================================================================================================================//

/// Many Layouts keep their child Items in a list.
struct ItemList final : public ItemContainer {
    virtual ~ItemList() override;

    virtual void clear() override;

    virtual void apply(std::function<void(Item*)> function) override;

    virtual bool contains(const Item* child) const override;

    virtual bool is_empty() const override { return items.empty(); }

    /// All Items contained in the list.
    std::vector<ItemPtr> items;
};

} // namespace detail

// ===================================================================================================================//

template<>
class Item::Access<detail::ItemContainer> {
    friend struct detail::ItemContainer;

    /// Constructor.
    Access(Item& item) : m_item(item) {}

    /// Sets the parent of this Item.
    /// @param is_orphaned   If the parent of the Item has already been deleted, the Item cannot unregister itself.
    void set_parent(Item* parent, bool is_orphaned) { m_item._set_parent(parent, is_orphaned); }

    /// The Item to access.
    Item& m_item;
};

} // namespace notf
