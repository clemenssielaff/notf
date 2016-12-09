#pragma once

#include <assert.h>
#include <atomic>
#include <memory>

#include "common/claim.hpp"
#include "common/signal.hpp"
#include "common/size2f.hpp"
#include "common/transform2.hpp"
#include "python/pyobject_wrapper.hpp"

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

class Layout;
class RenderLayer;
class Widget;
class Window;

/** Coordinate Spaces to pass to get_transform(). */
enum class Space : unsigned char {
    LOCAL, // returns transform in local coordinates, relative to the parent Item
    WINDOW, // returns transform in global coordinates, relative to the Window
    SCREEN, // returns transform in screen coordinates, relative to the screen origin
};

/** Unqiue identification token of an Item. */
using ItemID = size_t;

/**********************************************************************************************************************/

/** A Item is an abstraction of an item in the Layout hierarchy.
 * Both Widget and all Layout subclasses inherit from it.
 */
class Item : public Signaler<Item>, public std::enable_shared_from_this<Item> {

    friend class Layout;
    friend class Widget;

public: // methods
    // no copy / assignment
    Item(const Item&) = delete;
    Item& operator=(const Item&) = delete;

    virtual ~Item();

    /** Application-unique ID of this Item. */
    ItemID get_id() const { return m_id; }

    bool has_parent() const { return !m_parent.expired(); }

    /** Tests, if this Item is a descendant of the given `ancestor`.
     * @param ancestor      Potential ancestor (non-onwing pointer is used for identification only).
     * @return              True iff `ancestor` is an ancestor of this Item, false otherwise.
     */
    bool has_ancestor(const Item* ancestor) const;

    /** Returns the parent Item containing this Item, may be invalid. */
    std::shared_ptr<const Layout> get_parent() const { return m_parent.lock(); }

    /** Returns the Window containing this Widget (can be null). */
    std::shared_ptr<Window> get_window() const;

    /** Returns the opacity of this Item in the range [0 -> 1]. */
    virtual float get_opacity() const = 0;

    /** Returns the unscaled size of this Item in pixels. */
    virtual const Size2f& get_size() const = 0;

    /** Returns this Item's transformation in the given space. */
    Transform2 get_transform(const Space space) const
    {
        Transform2 result = Transform2::identity();
        switch (space) {
        case Space::WINDOW:
            _get_window_transform(result);
            break;

        case Space::SCREEN:
            result = _get_screen_transform();
            break;

        case Space::LOCAL:
            result = _get_local_transform();
            break;

        default:
            assert(0);
        }
        return result;
    }

    /** The current Claim of this Item. */
    virtual const Claim& get_claim() const = 0;

    /** Checks, if the Item is currently visible. */
    bool is_visible() const
    {
        return !(false
                 || get_size().is_zero()
                 || !get_size().is_valid()
                 || get_opacity() == approx(0));
    }

    /** Looks for a Widget at a given position in parent space.
     * @param local_pos     Local coordinates where to look for the Widget.
     * @return              The Widget at a given local position or an empty shared_ptr if there is none.
     */
    virtual std::shared_ptr<Widget> get_widget_at(const Vector2& local_pos) = 0;

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

    /** Emitted, when the opacity of this Item has changed.
     * @param New visiblity.
     */
    Signal<float> opacity_changed;

    /** Emitted, when the size of this Item has changed.
     * @param New size.
     */
    Signal<Size2f> size_changed;

    /** Emitted, when the transform of this Item has changed.
     * @param New local transform.
     */
    Signal<Transform2> transform_changed;

protected: // methods
    explicit Item();

    /** Tells the Window that its contents need to be redrawn. */
    bool _redraw();

    /** Sets the opacity of this Item.
     * @return      True iff the opacity has been modified.
     */
    virtual bool _set_opacity(float opacity) = 0;

    /** Updates the size of this Item.
     * Layouts use this function to update their items.
     * @return      True iff the size has been modified.
     */
    virtual bool _set_size(const Size2f size) = 0;

    /** Updates the transformation of this Item.
     * @return      True iff the transform has been modified.
     */
    virtual bool _set_transform(const Transform2 transform) = 0;

    /** Notifies the parent Layout that the Claim of this Item has changed.
     * Propagates up the Layout hierarchy to the first ancestor that doesn't need to change its Claim.
     */
    void _update_parent_layout();

    /** Stores the Python subclass object of this Item, if it was created through Python. */
    void _set_pyobject(PyObject* object);

protected: // static methods
    /** Allows any Item subclass to call _set_size on any other Item. */
    static void _set_item_size(Item* item, const Size2f size) { item->_set_size(std::move(size)); }

    /** Allows any Item subclass to call _set_item_transform on any other Item. */
    static void _set_item_transform(Item* item, const Transform2 transform)
    {
        item->_set_transform(std::move(transform));
    }

private: // methods
    /** Sets a new Item to contain this Item.
     * Setting a new parent makes this Item appear on top of the new parent.
     * I chose to do this since it is expected behaviour and reparenting + changing the z-value is a more common
     * use-case than reparenting the Item without changing its depth.
     */
    void _set_parent(std::shared_ptr<Layout> parent);

    /** Removes the current parent of this Item. */
    void _unparent() { _set_parent({}); }

    /** Recursive implementation to produce the Item's transformation in window space. */
    void _get_window_transform(Transform2& result) const;

    /** Returns the Item's transformation in screen space. */
    Transform2 _get_screen_transform() const;

    /** Returns the Item's transformation in parent space. */
    virtual Transform2 _get_local_transform() const = 0;

private: // static methods
    /** Returns the next, free ItemID.
     * Is thread-safe.
     */
    ItemID _get_next_id();

private: // fields
    /** Application-unique ID of this Item. */
    const ItemID m_id;

    /** The parent Item, may be invalid. */
    std::weak_ptr<Layout> m_parent;

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
