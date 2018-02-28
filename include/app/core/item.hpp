#pragma once

#include "app/forwards.hpp"
#include "common/id.hpp"
#include "common/meta.hpp"
#include "common/signal.hpp"

namespace notf {

//====================================================================================================================//

/// Unique identification token of an Item.
using ItemID = IdType<Item, size_t>;

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
/// Items in Python
/// ===============
/// When it comes to the lifetime of items, the way that Python and NoTF work together poses an interesting challenge.
///
/// Within NoTF's C++ codebase, Items (Layouts, Widgets, Controllers) are owned by their parent item through a shared
/// pointer.
/// There may be other shared pointers at any given time that keep an Item alive but they are usually short-lived and
/// only serve as a fail-save mechanism to ensure that the item stays valid between function calls.
/// When instancing a Item subclass in Python, a `PyObject` is generated, whose lifetime is managed by Python.
/// In order to have save access to the underlying NoTF Item, the `PyObject` keeps a shared pointer to it.
///
/// The difficulties arise, when Python execution of the app's `main.py` has finished.
/// By that time, all `PyObject`s are deallocated and release their shared pointers.
/// Items that are connected into the Item hierarchy stay alive, Items that are only referenced by a `PyObject` (and are
/// not part of the Item hierarchy) are deleted.
/// Usually that is what we would want, but in order to be able to call overridden virtual methods of Item subclasses
/// created in Python, we need to keep the `PyObject` alive for as long as the corresponding NoTF Item is alive.
///
/// To do that, Items can keep a `PyObject` around, making sure that it is alive as long as they are.
/// However, since the `PyObject` also has a shared pointer to the item, they own each other and will therefore never
/// be deleted.
/// Something has to give.
/// We cannot *not* keep a `PyObject` around, and we cannot allow the `PyObject` to have anything but a strong (owning)
/// pointer to an Item, because it would be destroyed immediately after creation from Python.
/// Therefore, all Python bindings of `Item` subclasses are outfitted with a custom deallocator function that
/// automatically stores a fresh reference to the `PyObject` in the `Item` instance, just as the `PyObject` would go out
/// of scope, if (and only if) there is another shared owner of the `Item` at the time of the `PyObjects` deallocation.
///
/// Effectively, this switches the ownership of the two objects, when the Python script execution has finished.
///
class Item : public receive_signals, public std::enable_shared_from_this<Item> {
    friend struct detail::ItemContainer;
    friend class WindowLayout;

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

    /// The Window containing the hierarchy that this Item is a part of.
    /// Is invalid if this Item is not part of a rooted hierarchy.
    Window* window() const { return m_window; }

    ///@{
    /// The parent of this Item.
    /// Is invalid if this Item does not have a parent.
    Item* parent() { return m_parent; }
    const Item* parent() const { return const_cast<Item*>(this)->parent(); }
    ///@}

    /// The (optional) name of this Item.
    const std::string& name() const { return m_name; }

    /// Checks if this Item is the parent of the given child.*/
    bool has_child(const Item* child) const;

    /// Checks if this Item has any children at all.
    bool has_children() const;

    /// Tests, if this Item is a descendant of the given ancestor Item.
    bool has_ancestor(const Item* ancestor) const;

    ///@{
    /// Finds and returns the first common ancestor of two Items, returns empty if none exists.
    Item* common_ancestor(Item* other);
    const Item* common_ancestor(const Item* other) const
    {
        return const_cast<Item*>(this)->common_ancestor(const_cast<Item*>(other));
    }
    ///@}

    ///@{
    /// Returns the closest Layout in the hierarchy of the given Item.
    /// Is empty if the given Item has no ancestor Layout.
    Layout* layout();
    const Layout* layout() const { return const_cast<Item*>(this)->layout(); }
    ///@}

    ///@{
    /// Returns the closest Controller in the hierarchy of the given Item.
    /// Is empty if the given Item has no ancestor Controller.
    Controller* controller();
    const Controller* controller() const { return const_cast<Item*>(this)->controller(); }
    ///@}

    ///@{
    /// Returns the ScreenItem associated with this given Item - either the Item itself or a Controller's root Item.
    /// Is empty if this is a Controller without a root Item.
    ScreenItem* screen_item();
    const ScreenItem* screen_item() const { return const_cast<Item*>(this)->screen_item(); }
    ///@}

    /// Updates the name of this Item.
    const std::string& set_name(const std::string name)
    {
        m_name = std::move(name);
        return m_name;
    }

protected:
    /// Removes a child Item from this Item.
    /// This needs to be a virtual method, because Items react differently to the removal of a child Item.
    virtual void _remove_child(const Item* child_item) = 0;

    /// Pulls new values from the parent if it changed.
    virtual void _update_from_parent();

    /// Changes the Window that this Item is displayed id.
    void _set_window(Window* window);

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
    static void _set_parent(Item* item, Item* parent) { item->_set_parent(parent); }

private:
    /// Sets the parent of this Item.
    /// @param is_orphaned   If the parent of the Item has already been deleted, the Item cannot unregister itself.
    void _set_parent(Item* parent, bool is_orphaned = false);

    // fields --------------------------------------------------------------------------------------------------------//
protected:
    /// All children of this Item.
    detail::ItemContainerPtr m_children;

private:
    /// Application-unique ID of this Item.
    const ItemID m_id;

    /// The Window containing the hierarchy that this Item is a part of.
    Window* m_window;

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

//====================================================================================================================//

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

    // fields --------------------------------------------------------------------------------------------------------//
protected:
    /// Sets the parent of all Items to nullptr without evoking proper reparenting.
    /// Is only used by the Item destructor.
    void destroy();
};

//====================================================================================================================//

/// Widgets have no child Items and use this empty Container as a placeholder instead.
struct EmptyItemContainer : public ItemContainer {
    virtual ~EmptyItemContainer() override;

    virtual void clear() override {}

    virtual void apply(std::function<void(Item*)>) override {}

    virtual bool contains(const Item*) const override { return false; }

    virtual bool is_empty() const override { return true; }
};

//====================================================================================================================//

/// Controller (and some Layouts) have a single child Item.
struct SingleItemContainer : public ItemContainer {
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
struct ItemList : public ItemContainer {
    virtual ~ItemList() override;

    virtual void clear() override;

    virtual void apply(std::function<void(Item*)> function) override;

    virtual bool contains(const Item* child) const override;

    virtual bool is_empty() const override { return items.empty(); }

    /// All Items contained in the list.
    std::vector<ItemPtr> items;
};

} // namespace detail

} // namespace notf
