#pragma once

#include <assert.h>
#include <memory>

#include "common/claim.hpp"
#include "common/signal.hpp"
#include "common/size2.hpp"
#include "common/xform2.hpp"
#include "utils/binding_accessors.hpp"

#ifdef NOTF_PYTHON
#include "python/py_fwd.hpp"
#endif

/* An Item is the base class for everything visual that makes up the UI.
 * It parents to specialized subclasses: Layout and Widgets.
 *
 * Item Hierarchy
 * ==============
 * Starting with the LayoutRoot, which is owned by a Window, every Item is owned by its immediate parent Item through
 * a shared pointer.
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
 * Within NoTF's C++ codebase, Items (Layouts, Widgets, Controllers) are usually owned by their parent item through a
 * `shared_ptr`.
 * There may be other `shared_ptr`s at any given time that keep an Item alive but they are usually short-lived and only
 * serve as a fail-save mechanism to ensure that the item stays valid between function calls.
 * When instancing a Item subclass in Python, a `PyObject` is generated, whose lifetime is managed by Python.
 * In order to have save access to the underlying NoTF Item, the `PyObject` keeps a `shared_ptr` to it.
 * So far so good.
 *
 * The difficulties arise, when Python execution of the app's `main.py` has finished.
 * By that time, all `PyObject`s are deallocated and release their `shared_ptr`s.
 * Items that are connected into the Item hierarchy stay alive, Items that are only referenced by a `PyObject` (and are
 * not part of the Item hierarchy) are deleted.
 * Usually that is what we would want, but in order to be able to call overridden virtual methods of Item subclasses
 * created in Python, we need to keep the `PyObject` alive for as long as the corresponding NoTF Item is alive.
 *
 * To do that, Items can keep a `PyObject` around, making sure that it is alive as long as they are.
 * However, since the `PyObject` also has a `std::shared_ptr` to the item, they own each other and will therefore never
 * be deleted.
 * Something has to give.
 * We cannot *not* keep a `PyObject` around, and we cannot allow the `PyObject` to have anything but a strong (owning)
 * pointer to an Item, because it would be destroyed immediately after creation from Python.
 * Therefore, all Python bindings of `Item` subclasses are outfitted with a custom deallocator function that
 * automatically stores a fresh reference to the `PyObject` in the `Item` instance, just as the `PyObject` would go out
 * of scope, if (and only if) there is another shared owner of the `Item` at the time of the `PyObjects` deallocation.
 * Effectively, this switches the ownership of the two objects.
 */

namespace notf {

class RenderLayer;
class Widget;
class Window;

/** Unqiue identification token of an Item. */
using ItemID = size_t;

/**********************************************************************************************************************/

/** A Item is an abstraction of an item in the Layout hierarchy.
 * All Widget and Layout subclasses inherit from it.
 */
class Item : public receive_signals, public std::enable_shared_from_this<Item> {

protected: // constructor
    /** Default Constructor. */
    explicit Item();

public: // methods
    // no copy / assignment
    Item(const Item&) = delete;
    Item& operator=(const Item&) = delete;

    /** Destructor */
    virtual ~Item();

    /** Application-unique ID of this Item. */
    ItemID id() const { return m_id; }

    /** Checks if this Item currently has a parent Item or not. */
    bool has_parent() const { return !m_parent.expired(); }

    /** Returns the parent Item containing this Item, may be invalid. */
    std::shared_ptr<const Item> parent() const { return m_parent.lock(); }

    /** Tests, if this Item is a descendant of the given `ancestor`.
     * @param ancestor  Potential ancestor (non-onwing pointer is used for identification only).
     * @return          True iff `ancestor` is an ancestor of this Item, false otherwise.
     */
    bool has_ancestor(const Item* ancestor) const;

    /** Returns the Window containing this Widget (can be empty). */
    std::shared_ptr<Window> window() const;

    /** Returns the Item's transformation in parent space. */
    const Xform2f& transform() const { return m_transform; }

    /** Recursive implementation to produce the Item's transformation in window space. */
    Xform2f window_transform() const;

    /** Returns the unscaled size of this Item in pixels. */
    const Size2f& size() const { return m_size; }

    /** Returns the opacity of this Item in the range [0 -> 1]. */
    float opacity() const { return m_opacity; }

    /** Sets the opacity of this Item.
     * @param opacity   Is clamped to range [0 -> 1] with 0 => fully transparent and 1 => fully opaque.
     * @return          True if the opacity changed, false if the old value is the same as the new one.
     */
    bool set_opacity(float opacity);

    /** The current Claim of this Item. */
    const Claim& claim() const { return m_claim; }

    /** Returns the current RenderLayer of this Item (can be empty). */
    const std::shared_ptr<RenderLayer>& render_layer() const { return m_render_layer; }

    /** (Re-)sets the RenderLayer of this Item.
     * Pass an empty shared_ptr to implicitly inherit the RenderLayer from the parent Layout.
     * If the Item does not have a parent Layout, this method does nothing but print a warning.
     * @param render_layer  New RenderLayer of this Item.
     */
    void set_render_layer(std::shared_ptr<RenderLayer> render_layer);

    /** Checks, if the Item is currently visible.
     * This method does return false if the opacity is zero but also if there are any other factors that make this Item
     * not visible, like a zero size for example.
     */
    bool is_visible() const
    {
        return !(false
                 || size().is_zero()
                 || !size().is_valid()
                 || opacity() <= precision_high<float>());
    }

    /** Looks for all Widgets at a given position in local space.
     * @param local_pos     Local coordinates where to look for a Widget.
     * @return              All Widgets at the given coordinate, ordered from front to back.
     */
    std::vector<Widget*> widgets_at(const Vector2f local_pos) const;

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
    Signal<Xform2f> transform_changed;

protected: // methods
    /** Notifies the parent Layout that the Claim of this Item has changed.
     * The change propagates up the Item hierarchy until it reaches the first ancestor, that doesn't need to change its
     * Claim, where it proceeds downwards again to re-layout all changed Items.
     */
    void _update_parent_layout();

    /** Tells the Window that this Item needs to be redrawn. */
    void _redraw();

    /** Updates the size of this Item.
     * Is virtual because Layouts can use this function to update their items.
     * @return      True iff the size has been modified.
     */
    virtual bool _set_size(const Size2f& size);

    /** Updates the transformation of this Item.
     * @return      True iff the transform has been modified.
     */
    bool _set_transform(const Xform2f transform);

    /** Updates the Claim but does not trigger any layouting.
     * @return      True iff the Claim was changed.
     */
    bool _set_claim(const Claim claim);

    /** Returns the first ancestor of this Item that has a specific type (can be empty if none is found). */
    template <typename AncestorType>
    std::shared_ptr<AncestorType> _first_ancestor() const
    {
        std::shared_ptr<Item> next = m_parent.lock();
        while (next) {
            if (std::shared_ptr<AncestorType> result = std::dynamic_pointer_cast<AncestorType>(next)) {
                return result;
            }
            next = next->m_parent.lock();
        }
        return {};
    }

#ifdef NOTF_PYTHON
    /** The Python object owned by this Item, is nullptr before the ownership is transferred from Python's __main__. */
    PyObject* _get_py_object() const { return m_py_object.get(); }

    // clang-format off
protected_except_for_bindings : // methods
    /** Stores the Python subclass object of this Item, if it was created through Python. */
    virtual void _set_pyobject(PyObject* object);
#endif

    // clang-format on
protected: // static methods
    /** Allows any Item subclass to call `_set_size` on any other Item. */
    static bool _set_item_size(Item* item, const Size2f& size) { return item->_set_size(size); }

    /** Allows any Item subclass to call `_set_item_transform` on any other Item. */
    static bool _set_item_transform(Item* item, const Xform2f transform) { return item->_set_transform(std::move(transform)); }

    /** Allows any Item subclass to call `_widgets_at` on any other Item. */
    static void _widgets_at_item_pos(Item* item, const Vector2f& local_pos, std::vector<Widget*>& result) { item->_widgets_at(local_pos, result); }

private: // methods
    /** Sets a new Item to manage this Item. */
    void _set_parent(std::shared_ptr<Item> parent);

    /** Removes the current parent of this Item. */
    void _unparent() { _set_parent({}); }

    /** Calculates the transformation of this Item relative to its Window. */
    void _window_transform(Xform2f& result) const;

    /** Recursive implementation to find all Widgets at a given position in local space */
    virtual void _widgets_at(const Vector2f& local_pos, std::vector<Widget*>& result) = 0;

private: // fields
    /** Application-unique ID of this Item. */
    const ItemID m_id;

    /** The parent Item, may be invalid. */
    std::weak_ptr<Item> m_parent;

    /** The RenderLayer of this Item.
     * An empty pointer means that this item inherits its render layer from its parent.
     */
    std::shared_ptr<RenderLayer> m_render_layer;

    /** Opacity of this Item in the range [0 -> 1]. */
    float m_opacity;

    /** Unscaled size of this Item in pixels. */
    Size2f m_size;

    /** 2D transformation of this Item in local space. */
    Xform2f m_transform;

    /** The Claim of a Item determines how much space it receives in the parent Layout. */
    Claim m_claim;

#ifdef NOTF_PYTHON
    /** Python subclass object of this Item, if it was created through Python. */
    std::unique_ptr<PyObject, decltype(&py_decref)> m_py_object;
#endif
};

} // namespace notf
