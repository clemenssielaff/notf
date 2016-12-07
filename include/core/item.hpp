#pragma once

#include <assert.h>
#include <atomic>
#include <memory>

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

class Claim;
class Layout;
class RenderLayer;
class Widget;
class Window;

/// @brief Visibility states, all but one mean that the LayoutItem is not visible, but all for different reasons.
enum class VISIBILITY : unsigned char {
    INVISIBLE, // LayoutItem is not displayed
    HIDDEN, // LayoutItem is not INVISIBLE but one of its parents is, so it cannot be displayed
    UNROOTED, // LayoutItem and all ancestors are not INVISIBLE, but the Widget is not a child of a RootWidget
    VISIBLE, // LayoutItem is displayed
};

/// @brief Coordinate Spaces to pass to get_transform().
enum class Space : unsigned char {
    PARENT, // returns transform in local coordinates, relative to the parent LayoutItem
    WINDOW, // returns transform in global coordinates, relative to the Window
    SCREEN, // returns transform in screen coordinates, relative to the screen origin
};

/** Unqiue identification token of an Item. */
using ItemID = size_t;

/**********************************************************************************************************************/

/** A LayoutItem is an abstraction of an item in the Layout hierarchy.
 * Both Widget and all Layout subclasses inherit from it.
 */
class Item : public Signaler<Item>, public std::enable_shared_from_this<Item> {

    friend class Layout;
    friend class Widget;

public: // abstract methods
    /// @brief Looks for a Widget at a given local position.
    /// @param local_pos    Local coordinates where to look for the Widget.
    /// @return The Widget at a given local position or an empty shared_ptr if there is none.
    virtual std::shared_ptr<Widget> get_widget_at(const Vector2& local_pos) = 0;

public: // methods
    /// no copy / assignment
    Item(const Item&) = delete;
    Item& operator=(const Item&) = delete;

    /// @brief Virtual destructor.
    virtual ~Item();

    /** Application-unique ID of this Item. */
    ItemID get_id() const { return m_id; }

    /// @brief Returns true iff this LayoutItem has a parent
    bool has_parent() const { return !m_parent.expired(); }

    /// @brief Tests, if this LayoutItem is a descendant of the given `ancestor`.
    /// @param ancestor Potential ancestor (non-onwing pointer is used for identification only).
    /// @return True iff `ancestor` is an ancestor of this LayoutItem, false otherwise.
    bool has_ancestor(const Item* ancestor) const;

    /// @brief Returns the parent LayoutItem containing this LayoutItem, may be invalid.
    std::shared_ptr<const Layout> get_parent() const { return m_parent.lock(); }

    /** Returns the Window containing this Widget (can be null). */
    std::shared_ptr<Window> get_window() const;

    /** The current Claim of this LayoutItem. */
    virtual const Claim& get_claim() const = 0;

    /** Check if this LayoutItem is visible or not. */
    bool is_visible() const { return m_visibility == VISIBILITY::VISIBLE; }

    /** The visibility of this LayoutItem. */
    VISIBILITY get_visibility() const { return m_visibility; }

    /** Returns the unscaled size of this LayoutItem in pixels. */
    const Size2f& get_size() const { return m_size; }

    /** Returns this LayoutItem's transformation in the given space. */
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

        case Space::PARENT:
            result = _get_parent_transform();
            break;

        default:
            assert(0);
        }
        return result;
    }

    /** Returns the current RenderLayer of this LayoutItem. Can be empty. */
    const std::shared_ptr<RenderLayer>& get_render_layer() const { return m_render_layer; }

    /** (Re-)sets the RenderLayer of this LayoutItem.
     * Pass an empty shared_ptr to implicitly inherit the RenderLayer from the parent Layout.
     */
    virtual void set_render_layer(std::shared_ptr<RenderLayer> render_layer)
    {
        m_render_layer = std::move(render_layer);
    }

public: // signals
    /// @brief Emitted when this LayoutItem got a new parent.
    /// @param ItemID of the new parent.
    Signal<ItemID> parent_changed;

    /// @brief Emitted, when the visibility of this LayoutItem has changed.
    /// @param New visiblity.
    Signal<VISIBILITY> visibility_changed;

    /// @brief Emitted when this LayoutItem' size changed.
    /// @param New size.
    Signal<Size2f> size_changed;

    /// @brief Emitted when this LayoutItem' transformation changed.
    /// @param New local transformation.
    Signal<Transform2> transform_changed;

protected: // methods
    explicit Item();

    /// @brief Shows (if possible) or hides this LayoutItem.
    void _set_visible(const bool is_visible);

    /** Updates the size of this LayoutItem.
     * Is virtual, since Layouts use this function to update their items.
     */
    virtual bool _set_size(const Size2f size)
    {
        if (size != m_size) {
            m_size = std::move(size);
            size_changed(m_size);
            _redraw();
            return true;
        }
        return false;
    }

    /** Updates the transformation of this LayoutItem. */
    bool _set_transform(const Transform2 transform)
    {
        if (transform != m_transform) {
            m_transform = std::move(transform);
            transform_changed(m_transform);
            _redraw();
            return true;
        }
        return false;
    }

    /** Notifies the parent Layout that the Claim of this Item has changed.
     * Propagates up the Layout hierarchy to the first ancestor that doesn't need to change its Claim.
     */
    void _update_parent_layout();

    /** Stores the Python subclass object of this Item, if it was created through Python. */
    void _set_pyobject(PyObject* object);

protected: // static methods
    /** Allows any LayoutItem subclass to call _set_size on any other LayoutItem. */
    static void _set_item_size(Item* item, const Size2f size) { item->_set_size(std::move(size)); }

    /** Allows any LayoutItem subclass to call _set_item_transform on any other LayoutItem. */
    static void _set_item_transform(Item* item, const Transform2 transform)
    {
        item->_set_transform(std::move(transform));
    }

private: // methods
    /** Tells the Window that its contents need to be redrawn. */
    virtual bool _redraw();

    /// @brief Sets a new LayoutItem to contain this LayoutItem.
    /// Setting a new parent makes this LayoutItem appear on top of the new parent.
    /// I chose to do this since it is expected behaviour and reparenting + changing the z-value is a more common
    /// use-case than reparenting the LayoutItem without changing its depth.
    void _set_parent(std::shared_ptr<Layout> parent);

    /// @brief Removes the current parent of this LayoutItem.
    void _unparent() { _set_parent({}); }

    /// @brief Recursive function to let all children emit visibility_changed when the parent's visibility changed.
    virtual void _cascade_visibility(const VISIBILITY visibility);

    /// @brief Recursive implementation to produce the LayoutItem's transformation in window space.
    void _get_window_transform(Transform2& result) const;

    /// @brief Returns the LayoutItem's transformation in screen space.
    Transform2 _get_screen_transform() const;

    /// @brief Returns the LayoutItem's transformation in parent space.
    Transform2 _get_parent_transform() const { return m_transform; }

private: // static methods
    /** Returns the next, free ItemID.
     * Is thread-safe.
     */
    ItemID _get_next_id();

private: // fields
    /** Application-unique ID of this Item. */
    const ItemID m_id;

    /// @brief The parent LayoutItem, may be invalid.
    std::weak_ptr<Layout> m_parent;

    /// @brief Visibility state of this LayoutItem.
    VISIBILITY m_visibility;

    /// @brief Unscaled size of this LayoutItem in pixels.
    Size2f m_size;

    /// @brief 2D transformation of this LayoutItem in local space.
    Transform2 m_transform;

    /** The RenderLayer of this LayoutItem.
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
