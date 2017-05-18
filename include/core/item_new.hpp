#pragma once

#include <memory>

#include "common/id.hpp"
#include "common/meta.hpp"
#include "common/signal.hpp"

#ifdef NOTF_PYTHON
#include "ext/python/py_fwd.hpp"
#include "utils/binding_accessors.hpp"
#endif

namespace notf {

/* Item hierarchy forward declarations */
class Controller;
class Item;
class Layout;
class ScreenItem;
class Widget;
class Window;

using ControllerPtr = std::shared_ptr<Controller>;
using ItemPtr       = std::shared_ptr<Item>;
using LayoutPtr     = std::shared_ptr<Layout>;
using ScreenItemPtr = std::shared_ptr<ScreenItem>;
using WidgetPtr     = std::shared_ptr<Widget>;

using ConstControllerPtr = std::shared_ptr<const Controller>;
using ConstItemPtr       = std::shared_ptr<const Item>;
using ConstLayoutPtr     = std::shared_ptr<const Layout>;
using ConstScreenItemPtr = std::shared_ptr<const ScreenItem>;
using ConstWidgetPtr     = std::shared_ptr<const Widget>;

/** Unqiue identification token of an Item. */
using RawID  = size_t;
using ItemID = Id<Item, RawID>;

/**********************************************************************************************************************/

/** An Item is the base class for all objects in the `Item hierarchy`.
 * Its three main spezializations are `Widgets`, `Layouts` and `Controllers`.
 *
 * Item Hierarchy
 * ==============
 * Starting with the WindowLayout at the root, which is owned by a Window, every Item is owned by its immediate parent
 * Item through a shared pointer.
 *
 * Item IDs
 * ========
 * Each Item has a constant unique integer ID assigned to it upon instanciation.
 * It can be used to identify the Item in a map, for debugging purposes or in conditionals.
 *
 * Items in Python
 * ===============
 * When it comes to the lifetime of items, the way that Python and NoTF work together poses an interesting challenge.
 *
 * Within NoTF's C++ codebase, Items (Layouts, Widgets, Controllers) are owned by their parent item through a shared
 * pointer.
 * There may be other shared pointers at any given time that keep an Item alive but they are usually short-lived and
 * only serve as a fail-save mechanism to ensure that the item stays valid between function calls.
 * When instancing a Item subclass in Python, a `PyObject` is generated, whose lifetime is managed by Python.
 * In order to have save access to the underlying NoTF Item, the `PyObject` keeps a shared pointer to it.
 *
 * The difficulties arise, when Python execution of the app's `main.py` has finished.
 * By that time, all `PyObject`s are deallocated and release their shared pointers.
 * Items that are connected into the Item hierarchy stay alive, Items that are only referenced by a `PyObject` (and are
 * not part of the Item hierarchy) are deleted.
 * Usually that is what we would want, but in order to be able to call overridden virtual methods of Item subclasses
 * created in Python, we need to keep the `PyObject` alive for as long as the corresponding NoTF Item is alive.
 *
 * To do that, Items can keep a `PyObject` around, making sure that it is alive as long as they are.
 * However, since the `PyObject` also has a shared pointer to the item, they own each other and will therefore never
 * be deleted.
 * Something has to give.
 * We cannot *not* keep a `PyObject` around, and we cannot allow the `PyObject` to have anything but a strong (owning)
 * pointer to an Item, because it would be destroyed immediately after creation from Python.
 * Therefore, all Python bindings of `Item` subclasses are outfitted with a custom deallocator function that
 * automatically stores a fresh reference to the `PyObject` in the `Item` instance, just as the `PyObject` would go out
 * of scope, if (and only if) there is another shared owner of the `Item` at the time of the `PyObjects` deallocation.
 *
 * Effectively, this switches the ownership of the two objects, when the Python script execution has finished.
 */
class Item : public receive_signals, public std::enable_shared_from_this<Item> {

protected: // constructor *********************************************************************************************/
    /** Default Constructor. */
    Item();

public: // methods ****************************************************************************************************/
    DISALLOW_COPY_AND_ASSIGN(Item)

    /** Destructor */
    virtual ~Item();

    /** Application-unique ID of this Item. */
    ItemID get_id() const { return m_id; }

    /** Checks if this Item currently has a parent Item or not. */
    bool has_parent() const { return !m_parent.expired(); }

    /** Returns the parent Item containing this Item, may be invalid. */
    ItemPtr get_parent() { return m_parent.lock(); }
    ConstItemPtr get_parent() const { return m_parent.lock(); }

    /** Tests, if this Item is a descendant of the given ancestor Item. */
    bool has_ancestor(const Item* ancestor) const;

    /** Finds and returns the first common ancestor of two Items, returns empty if none exists. */
    ItemPtr get_common_ancestor(Item* other);
    ConstItemPtr get_common_ancestor(const Item* other) const
    {
        return const_cast<Item*>(this)->get_common_ancestor(const_cast<Item*>(other));
    }

    /** Returns the closest Layout in the hierarchy of the given Item.
     * Is empty if the given Item has no ancestor Layout.
     */
    LayoutPtr get_layout();
    ConstLayoutPtr get_layout() const { return const_cast<Item*>(this)->get_layout(); }

    /** Returns the closest Controller in the hierarchy of the given Item.
     * Is empty if the given Item has no ancestor Controller.
     */
    ControllerPtr get_controller();
    ConstControllerPtr get_controller() const { return const_cast<Item*>(this)->get_controller(); }

    /** Returns the ScreenItem associated with this given Item - either the Item itself or a Controller's root Item. */
    ScreenItemPtr get_screen_item();
    ConstScreenItemPtr get_screen_item() const { return const_cast<Item*>(this)->get_screen_item(); }

    /** Returns the Window containing this Widget (can be empty). */
    Window* get_window() const;

public: // signals ****************************************************************************************************/
    /** Emitted when this Item got a new parent.
     * @param ItemID of the new parent.
     */
    Signal<ItemID> on_parent_changed;

protected: // methods *************************************************************************************************/
    /** Returns the first ancestor of this Item that has a specific type (can be empty if none is found). */
    template <typename Type>
    std::shared_ptr<Type> _get_first_ancestor() const;

#ifdef NOTF_PYTHON
    /** The Python object owned by this Item, is nullptr before the ownership is transferred from Python's __main__. */
    PyObject* _get_py_object() const { return m_py_object.get(); }

    // clang-format off
protected_except_for_bindings : // methods
    /** Stores the Python subclass object of this Item, if it was created through Python. */
    virtual void _set_pyobject(PyObject* object);
#endif
    // clang-format on

protected: // static methods ******************************************************************************************/
    /** Allow any Item to inspect the raw parent of every other Item. */
    Item* _get_raw_parent(const Item* item) { return item->m_raw_parent; }

private: // fields ****************************************************************************************************/
    /** Application-unique ID of this Item. */
    const ItemID m_id;

    /** The parent Item, may be invalid. */
    std::weak_ptr<Item> m_parent;

    /** The parent Item as a raw pointer for quick traversal.
     * Must only be used internally and only when you are certain that the pointer is still valid.
     */
    Item* m_raw_parent;

#ifdef NOTF_PYTHON
    /** Python subclass object of this Item, if it was created through Python. */
    std::unique_ptr<PyObject, decltype(&py_decref)> m_py_object;
#endif
};

/**********************************************************************************************************************/

/** Convenience function to create correctly typed `shared_from_this` shared_ptrs from Item subclasses. */
template <typename ItemSubclass, ENABLE_IF_SUBCLASS(ItemSubclass, Item)>
std::shared_ptr<ItemSubclass> make_shared_from(ItemSubclass* item)
{
    return std::dynamic_pointer_cast<ItemSubclass>(static_cast<Item*>(item)->shared_from_this());
}

} // namespace notf
