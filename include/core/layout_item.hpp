#pragma once

#include <assert.h>
#include <memory>

#include "common/claim.hpp"
#include "common/signal.hpp"
#include "common/size2f.hpp"
#include "common/transform2.hpp"
#include "core/object.hpp"

namespace notf {

class Layout;
class RenderLayer;
class Widget;

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

/**********************************************************************************************************************/

/** A LayoutItem is an abstraction of an item in the Layout hierarchy.
 * Both Widget and all Layout subclasses inherit from it.
 */
class LayoutItem : public Object, public Signaler<LayoutItem> {

    friend class Layout;

public: // abstract methods
    /// @brief Looks for a Widget at a given local position.
    /// @param local_pos    Local coordinates where to look for the Widget.
    /// @return The Widget at a given local position or an empty shared_ptr if there is none.
    virtual std::shared_ptr<Widget> get_widget_at(const Vector2& local_pos) = 0;

public: // methods
    /// @brief Virtual destructor.
    virtual ~LayoutItem() override;

    /// @brief Returns true iff this LayoutItem has a parent
    bool has_parent() const { return !m_parent.expired(); }

    /// @brief Tests, if this LayoutItem is a descendant of the given `ancestor`.
    /// @param ancestor Potential ancestor (non-onwing pointer is used for identification only).
    /// @return True iff `ancestor` is an ancestor of this LayoutItem, false otherwise.
    bool has_ancestor(const LayoutItem* ancestor) const;

    /// @brief Returns the parent LayoutItem containing this LayoutItem, may be invalid.
    std::shared_ptr<const Layout> get_parent() const { return m_parent.lock(); }

    /// @brief Returns the Window containing this Widget (can be null).
    std::shared_ptr<Window> get_window() const;

    /// @brief The current Claim of this LayoutItem.
    const Claim& get_claim() const { return m_claim; }

    /** Check if this LayoutItem is visible or not. */
    bool is_visible() const { return m_visibility == VISIBILITY::VISIBLE; }

    /** The visibility of this LayoutItem. */
    VISIBILITY get_visibility() const { return m_visibility; }

    /// @brief Returns the unscaled size of this LayoutItem in pixels.
    const Size2f& get_size() const { return m_size; }

    /// @brief Returns this LayoutItem's transformation in the given space.
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
    /// @param Handle of the new parent.
    Signal<Handle> parent_changed;

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
    /// @brief Value Constructor.
    /// @param handle   Application-unique Handle of this Item.
    explicit LayoutItem(const Handle handle);

    /** Tells the Window that its contents need to be redrawn. */
    virtual bool _redraw();

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

    /** Updates the Claim but does not trigger any layouting. */
    bool _set_claim(const Claim claim)
    {
        if (claim != m_claim) {
            m_claim = std::move(claim);
            return true;
        }
        return false;
    }

    /// @brief Notifies the parent Layout that the Claim of this Item has changed.
    /// Propagates up the Layout hierarchy to the first ancestor that doesn't need to change its Claim.
    void _update_parent_layout();

protected: // static methods
    /** Allows any LayoutItem subclass to call _set_size on any other LayoutItem. */
    static void _set_item_size(LayoutItem* item, const Size2f size) { item->_set_size(std::move(size)); }

    /** Allows any LayoutItem subclass to call _set_item_transform on any other LayoutItem. */
    static void _set_item_transform(LayoutItem* item, const Transform2 transform)
    {
        item->_set_transform(std::move(transform));
    }

private: // methods
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

private: // fields
    /// @brief The parent LayoutItem, may be invalid.
    std::weak_ptr<Layout> m_parent;

    /// @brief Visibility state of this LayoutItem.
    VISIBILITY m_visibility;

    /// @brief The Claim of a LayoutItem determines how much space it receives in the parent Layout.
    Claim m_claim;

    /// @brief Unscaled size of this LayoutItem in pixels.
    Size2f m_size;

    /// @brief 2D transformation of this LayoutItem in local space.
    Transform2 m_transform;

    /** The RenderLayer of this LayoutItem.
     * An empty pointer means that this item inherits its render layer from its parent.
     */
    std::shared_ptr<RenderLayer> m_render_layer;
};

} // namespace notf
