#pragma once

#include <assert.h>
#include <atomic>
#include <memory>

#include "common/claim.hpp"
#include "common/signal.hpp"
#include "common/size2f.hpp"
#include "common/transform2.hpp"
#include "python/py_fwd.hpp"
#include "utils/binding_accessors.hpp"

/*
 * When it comes to the lifetime of items, the way that Python and NoTF work together poses an interesting challenge.
 *
 * Within NoTF's C++ codebase, items (Layouts, Widgets, Controllers) are usually owned by their parent item through a
 * `shared_ptr`.
 * There may be other `shared_ptr`s at any given time that keep an item alive but they are usually short-lived and only
 * serve as a fail-save mechanism to ensure that the item stays valid between to function calls.
 * When subclassing an item from Python, a `PyObject` is generated, whose lifetime is managed by Python.
 * In order to have save access to the underlying item, the `PyObject` keeps a `shared_ptr` to it.
 * So far so good.
 *
 * The difficulties arise, when Python execution of the app's `main.py` has finished.
 * By that time, all `PyObject`s are deallocated and release their `shared_ptr`s.
 * Items that are connected into the item hierarchy stay alive, items that are only referenced by a `PyObject` (and are
 * not part of the Item hierarchy) are deleted.
 * Usually that is what we would want, but in order to be able to call overridden virtual methods of item subclasses
 * created in Python, we need to keep the `PyObject` alive for as long as the corresponding NoTF item is alive.
 *
 * To do that, items can keep a `PyObject` around, making sure that it is alive as long as they are.
 * However, since the `PyObject` also has a `std::shared_ptr` to the item, they own each other and will therefore never
 * be deleted.
 * Something has to give.
 * We cannot *not* keep a `PyObject` around, and we cannot allow the `PyObject` to have anything but a strong (owning)
 * pointer to an item, because it would be destroyed immediately after creation from Python.
 * Therefore, all Python bindings of `item` subclasses are outfitted with a custom deallocator function that
 * automatically stores a fresh reference to the `PyObject` in the `item` instance, just as the `PyObject` would go out
 * of scope, if (and only if) there is another shared owner of the `item` at the time of the `PyObjects` deallocation.
 * Effectively, this switches the ownership of the two objects.
 */

namespace notf {

class AbstractController;
class Layout;
class LayoutItem;
class RenderLayer;
class Widget;
class Window;

/** Unqiue identification token of an Item. */
using ItemID = size_t;

/**********************************************************************************************************************/

/** A Item is an abstraction of an item in the Layout hierarchy.
 * Both Widget and all Layout subclasses inherit from it.
 */
class Item : public receive_signals<Item>, public std::enable_shared_from_this<Item> {

    friend class AbstractController;
    friend class Layout;
    friend class Widget;

public: // methods
    // no copy / assignment
    Item(const Item&) = delete;
    Item& operator=(const Item&) = delete;

    virtual ~Item();

    /** Application-unique ID of this Item. */
    ItemID get_id() const { return m_id; }

    /** Returns the LayoutItem associated with this Item.
     * Items that are already LayoutItems return themselves, Controller return their root LayoutItem or nullptr.
     */
    virtual LayoutItem* get_layout_item() = 0;
    virtual const LayoutItem* get_layout_item() const = 0;

    /** Checks if this Item currently has a parent Item or not. */
    bool has_parent() const { return !m_parent.expired(); }

    /** Tests, if this Item is a descendant of the given `ancestor`.
     * @param ancestor      Potential ancestor (non-onwing pointer is used for identification only).
     * @return              True iff `ancestor` is an ancestor of this Item, false otherwise.
     */
    bool has_ancestor(const Item* ancestor) const;

    /** Returns the parent Item containing this Item, may be invalid. */
    std::shared_ptr<const Item> get_parent() const { return m_parent.lock(); }

    /** Returns the Controller managing this Item. */
    std::shared_ptr<AbstractController> get_controller() const;

    /** Returns the Window containing this Widget (can be null). */
    std::shared_ptr<Window> get_window() const;

    /** Returns the current RenderLayer of this Item. Can be empty. */
    const std::shared_ptr<RenderLayer>& get_render_layer() const { return m_render_layer; }

    /** (Re-)sets the RenderLayer of this Item.
     * Pass an empty shared_ptr to implicitly inherit the RenderLayer from the parent Layout.
     */
    virtual void set_render_layer(std::shared_ptr<RenderLayer> render_layer)
    {
        m_render_layer = std::move(render_layer);
    }

public: // signals
    /** Emitted when this Item got a new parent.
     * @param ItemID of the new parent.
     */
    Signal<ItemID> parent_changed;

protected: // methods
    explicit Item();

    /** Returns the first ancestor of this Item that has a specific type. */
    template <typename T>
    std::shared_ptr<T> _get_first_ancestor() const;

    /** Notifies the parent Layout that the Claim of this Item has changed.
     * Propagates up the Layout hierarchy to the first ancestor that doesn't need to change its Claim.
     */
    void _update_parent_layout();

protected: // static methods
    /** Allows any Item subclass to call _set_size on any other Item. */
    static bool _set_item_size(LayoutItem* layout_item, const Size2f size);

    /** Allows any Item subclass to call _set_item_transform on any other Item. */
    static bool _set_item_transform(LayoutItem* layout_item, const Transform2 transform);

private: // methods
    /** Sets a new Item to manage this Item. */
    void _set_parent(std::shared_ptr<Item> parent);

    /** Removes the current parent of this Item. */
    void _unparent() { _set_parent({}); }

    // clang-format off
protected_except_for_bindings : // methods
    /** Stores the Python subclass object of this Item, if it was created through Python. */
    void _set_pyobject(PyObject* object);

    // clang-format on
private: // static methods
    /** Returns the next, free ItemID.
     * Is thread-safe.
     */
    ItemID _get_next_id();

private: // fields
    /** Application-unique ID of this Item. */
    const ItemID m_id;

    /** The parent Item, may be invalid. */
    std::weak_ptr<Item> m_parent;

    /** The RenderLayer of this Item.
     * An empty pointer means that this item inherits its render layer from its parent.
     */
    std::shared_ptr<RenderLayer> m_render_layer;

    /** Python subclass object of this Item, if it was created through Python. */
    std::unique_ptr<PyObject, decltype(&py_decref)> py_object;

private: // static fields
    /** The next available Item ID, is ever-increasing. */
    static std::atomic<ItemID> s_nextID;
};

} // namespace notf
