#pragma once

#include <assert.h>
#include <memory>

#include "common/claim.hpp"
#include "common/id.hpp"
#include "common/signal.hpp"
#include "common/size2.hpp"
#include "common/xform2.hpp"
#include "utils/binding_accessors.hpp"

#ifdef NOTF_PYTHON
#include "python/py_fwd.hpp"
#endif

namespace notf {

class Item;
class ScreenItem;
class Widget;
class Layout;
class Controller;

using ItemPtr       = std::shared_ptr<Item>;
using ScreenItemPtr = std::shared_ptr<ScreenItem>;
using WidgetPtr     = std::shared_ptr<Widget>;
using LayoutPtr     = std::shared_ptr<Layout>;
using ControllerPtr = std::shared_ptr<Controller>;

using ConstItemPtr       = std::shared_ptr<const Item>;
using ConstScreenItemPtr = std::shared_ptr<const ScreenItem>;
using ConstWidgetPtr     = std::shared_ptr<const Widget>;
using ConstLayoutPtr     = std::shared_ptr<const Layout>;
using ConstControllerPtr = std::shared_ptr<const Controller>;

class RenderLayer;
class Window;

/** Unqiue identification token of an Item. */
using RawID  = size_t;
using ItemID = Id<Item, RawID>;

/**********************************************************************************************************************/

/* An Item is the base class for all objects in the `Item hierarchy`.
 * Its three main spezializations are `Widgets`, `Layouts` and `Controllers`.
 *
 * Item Hierarchy
 * ==============
 * Starting with the WindowLayout, which is owned by a Window, every Item is owned by its immediate parent Item through
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
 *
 * RenderLayer
 * ===========
 * By default, it is the Layouts' job to determine the order in which Widgets are drawn on screen.
 * This will probably suffice for most use cases, is very easy to reason about and fast to compute.
 * However, we might want to make exceptions to this rule, where an item (for example a tooltip) is logically part of
 * a nested Layout, but should be drawn on top of everything else.
 * For that, we have RenderLayers, explicit layers that Item in the hierarchy can be assigned to in order to render
 * them before or after other parts of the hierarchy have been rendered.
 * The WindowLayout is part of RenderLayer `zero` which is the default one.
 * If you set an Item to another RenderLayer (for example `one`) it, and all of its children will be drawn in front of
 * everything in RenderLayer zero.
 * Internally, this is done by setting the `m_render_layer` member to point either to a RenderLayer object or be empty,
 * in which case the Item will simply inherit the parent's RenderLayer and so on.
 * If you change nothing by hand, all of the pointers (except the one for the root WindowLayout) will be empty and all
 * Items in the Window's hierachy will be implicit members of the default render layer.
 */
class Item : public receive_signals, public std::enable_shared_from_this<Item> {

    friend class Controller;   // can parent ScreenItems
    friend class Layout;       // can parent ScreenItems
    friend class WindowLayout; // can set its RenderLayer even though it has no parent

public: // static functions *******************************************************************************************/
    /** Returns the ScreenItem associated with a given Item - either the Item itself or a Controller's root Item. */
    static ScreenItem* get_screen_item(Item* item);

    /** Returns the ScreenItem associated with a given Item - either the Item itself or a Controller's root Item. */
    static const ScreenItem* get_screen_item(const Item* item) { return get_screen_item(const_cast<Item*>(item)); }

protected: // constructor *********************************************************************************************/
    /** Default Constructor. */
    Item();

public: // methods ****************************************************************************************************/
    // no copy / assignment
    Item(const Item&) = delete;
    Item& operator=(const Item&) = delete;

    /** Destructor */
    virtual ~Item();

    /** Application-unique ID of this Item. */
    ItemID get_id() const { return m_id; }

    /** Checks if this Item currently has a parent Item or not. */
    bool has_parent() const { return !m_parent.expired(); }

    /** Returns the parent Item containing this Item, may be invalid. */
    ItemPtr get_parent() const { return m_parent.lock(); }

    /** Returns the Layout into which this Item is embedded.
     * The only Item without a Layout is the WindowLayout.
     */
    LayoutPtr get_layout() const { return _get_layout(); }

    /** Returns the Controller managing this Item.
     * The only Item without a Controller is the WindowLayout.
     */
    ControllerPtr get_controller() const;

    /** Tests, if this Item is a descendant of the given `ancestor`.
     * @param ancestor  Potential ancestor.
     * @return          True iff `ancestor` is an ancestor of this Item, false otherwise.
     */
    bool has_ancestor(const ItemPtr& ancestor) const;

    /** Returns the Window containing this Widget (can be empty). */
    std::shared_ptr<Window> get_window() const;

    /** Returns the current RenderLayer of this Item.
     * If `own==false`, the returned RenderLayer is valid if this Item is in the Item hierarchy and invalid if not.
     * If `own==true`, it is empty if the Item uses its parent's RenderLayer or if it is not part of the Item hierarchy.
     * @param own   By default, the first valid RenderLayer in the ancestry is returned, if `own==true` the RenderLayer
     *              of this Item is returned, even if it is empty.
     */
    const std::shared_ptr<RenderLayer>& get_render_layer(const bool own = false) const;

    /** (Re-)sets the RenderLayer of this Item.
     * Pass an empty shared_ptr to implicitly inherit the RenderLayer from the parent Layout.
     * If the Item does not have a parent, this method does nothing but print a warning.
     * @param render_layer  New RenderLayer of this Item.
     */
    void set_render_layer(std::shared_ptr<RenderLayer> render_layer);

public: // signals ****************************************************************************************************/
    /** Emitted when this Item got a new parent.
     * @param ItemID of the new parent.
     */
    Signal<ItemID> parent_changed;

protected: // methods *************************************************************************************************/
    /** Returns the Layout into which this Item is embedded as a mutable pointer.
     * The only Item without a Layout is the WindowLayout.
     */
    LayoutPtr _get_layout() const;

    /** Returns the Controller managing this Item as a mutable pointer.
     * The only Item without a Controller is the WindowLayout.
     */
    ControllerPtr _get_controller() const;

    /** Notifies the parent Layout that the Claim of this Item has changed.
     * The change propagates up the Item hierarchy until it reaches the first ancestor, that doesn't need to change its
     * Claim, where it proceeds downwards again to re-layout all changed Items.
     */
    void _update_parent_layout();

    /** Returns the first ancestor of this Item that has a specific type (can be empty if none is found). */
    template <typename AncestorType>
    std::shared_ptr<AncestorType> _get_first_ancestor() const;

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
    /** Allows any Item subclass to call `_widgets_at` on any other Item. */
    static void _get_widgets_at_item_pos(const Item* item, const Vector2f& local_pos, std::vector<Widget*>& result)
    {
        item->_get_widgets_at(local_pos, result);
    }

private: // methods
    /** Sets a new Item to manage this Item or an empty pointer to unparent it. */
    void _set_parent(std::shared_ptr<Item> parent);

    /** Recursive implementation to find all Widgets at a given position in local space
     * @param local_pos     Local coordinates where to look for a Widget.
     * @return              All Widgets at the given coordinate, ordered from front to back.
     */
    virtual void _get_widgets_at(const Vector2f& local_pos, std::vector<Widget*>& result) const = 0;

private: // fields ****************************************************************************************************/
    /** Application-unique ID of this Item. */
    const ItemID m_id;

    /** The parent Item, may be invalid. */
    std::weak_ptr<Item> m_parent;

    /** The RenderLayer of this Item.
     * An empty pointer means that this item inherits its render layer from its parent.
     */
    std::shared_ptr<RenderLayer> m_render_layer;

#ifdef NOTF_PYTHON
    /** Python subclass object of this Item, if it was created through Python. */
    std::unique_ptr<PyObject, decltype(&py_decref)> m_py_object;
#endif
};

} // namespace notf
