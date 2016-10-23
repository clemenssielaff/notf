#pragma once

#include <assert.h>
#include <memory>
#include <unordered_map>

#include "common/signal.hpp"
#include "common/size2r.hpp"
#include "common/transform2.hpp"
#include "core/abstract_object.hpp"

namespace notf {

class Layout;
class LayoutRoot;
class Widget;

/// @brief Visibility states, all but one mean that the LayoutItem is not visible, but all for different reasons.
enum class VISIBILITY : unsigned char {
    INVISIBLE, // LayoutItem is not displayed
    HIDDEN, // LayoutItem is not INVISIBLE but one of its parents is, so it cannot be displayed
    UNROOTED, // LayoutItem and all ancestors are not INVISIBLE, but the Widget is not a child of a RootWidget
    VISIBLE, // LayoutItem is displayed
};

/// @brief Coordinate Spaces to pass to get_transform().
enum class SPACE : unsigned char {
    PARENT, // returns transform in local coordinates, relative to the parent LayoutItem
    WINDOW, // returns transform in global coordinates, relative to the Window
    SCREEN, // returns transform in screen coordinates, relative to the screen origin
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/**
 * @brief A LayoutItem is an abstraction of an item in the Layout hierarchy.
 *
 * Both Widget and all Layout subclasses inherit from it.
 */
class LayoutItem : public AbstractObject, public Signaler<LayoutItem> {

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
    /// @param ancestor Potential ancestor
    /// @return True iff `ancestor` is an ancestor of this LayoutItem, false otherwise.
    bool has_ancestor(const std::shared_ptr<LayoutItem>& ancestor) const;

    /// @brief Returns the parent LayoutItem containing this LayoutItem, may be invalid.
    std::shared_ptr<const Layout> get_parent() const { return m_parent.lock(); }

    /// @brief Returns the LayoutItem of the hierarchy containing this LayoutItem.
    /// Is invalid if this LayoutItem is unrooted.
    std::shared_ptr<const LayoutRoot> get_root() const;

    /// @brief Checks the visibility of this LayoutItem.
    VISIBILITY get_visibility() const { return m_visibility; }

    /// @brief Returns the unscaled size of this LayoutItem in pixels.
    const Size2r& get_size() const { return m_size; }

    /// @brief Returns this LayoutItem's transformation in the given space.
    Transform2 get_transform(const SPACE space) const
    {
        Transform2 result = Transform2::identity();
        switch (space) {
        case SPACE::WINDOW:
            get_window_transform(result);
            break;

        case SPACE::SCREEN:
            result = get_screen_transform();
            break;

        case SPACE::PARENT:
            result = get_parent_transform();
            break;

        default:
            assert(0);
        }
        return result;
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
    Signal<Size2r> size_changed;

protected: // methods
    /// @brief Value Constructor.
    /// @param handle   Application-unique Handle of this Item.
    explicit LayoutItem(const Handle handle)
        : AbstractObject(handle)
        , m_parent()
        , m_visibility(VISIBILITY::VISIBLE)
        , m_size()
        , m_transform(Transform2::identity())
    {
    }

    /// @brief Shows (if possible) or hides this LayoutItem.
    void set_visible(const bool is_visible);

    /// @brief Updates the size of this LayoutItem.
    void set_size(const Size2r size)
    {
        m_size = size;
        size_changed(m_size);
    }

    /// @brief Tells this LayoutItem and all of its children to redraw.
    virtual void redraw() = 0;

    /// @brief Propagates a layout change upwards to the first ancestor that doesn't need to change its size.
    /// Then continues to spread down again through all children of that ancestor.
    void relayout_up();

private: // methods
    /// @brief Sets a new LayoutItem to contain this LayoutItem.
    void set_parent(std::shared_ptr<Layout> parent);

    /// @brief Removes the current parent of this LayoutItem.
    void unparent() { set_parent({}); }

    /// @brief Recursive function to let all children emit visibility_changed when the parent's visibility changed.
    virtual void cascade_visibility(const VISIBILITY visibility);

    /// @brief Recursive implementation to produce the LayoutItem's transformation in window space.
    void get_window_transform(Transform2& result) const;

    /// @brief Returns the LayoutItem's transformation in screen space.
    Transform2 get_screen_transform() const;

    /// @brief Returns the LayoutItem's transformation in parent space.
    Transform2 get_parent_transform() const { return m_transform; }

private: // fields
    /// @brief The parent LayoutItem, may be invalid.
    std::weak_ptr<Layout> m_parent;

    /// @brief Visibility state of this LayoutItem.
    VISIBILITY m_visibility;

    /// @brief Unscaled size of this LayoutItem in pixels.
    Size2r m_size;

    /// @brief 2D transformation of this LayoutItem in local space.
    Transform2 m_transform;
};

} // namespace notf

/*
 * We have to things going on here: redraw and update.
 *
 * A redraw means, that the Widget registers itself with the Render Manager to be drawn next frame.
 * Currently, all Widgets (the one that is displayed) is drawn each frame, so worrying about the redraw method now
 * is a bit premature.
 * Still, there is the question of how we can minimize redraws.
 * I like the idea that only those Widgets that really change (dirty) register themselves with the Manager.
 * Just before rendering, the Manager than figures out the min. set of Widgets that need to be drawn, in what order etc.
 * It also takes into consideration that non-dirty Widgets may need to be drawn if, for example, part of them was
 * revealed by another Widget that was in front of them on the previous frame but has now moved away.
 * Much like the dirtying in Ambani, there a few 'root' dirt Widgets that register themselves and the RenderManager
 * extrapolates from there.
 * Actually, since all Widgets must now fully encapsulate their children, we can at least determine that if a parent
 * is redrawn, that all of its children must be redrawn as well just because they are drawn onto the parent's canvas.
 *
 * Updating means that the size or setup of the Widget has changed and that it in turn might have to re-arrange its
 * children and parent.
 * An update will most likely trigger a redraw, but we'll ignore that for now.
 * An update works potentially in both directions, both up and down the Widget hierarchy.
 * This is potentially dangerous because it can lead to cycles.
 * What we need is that a child notices that its size has changed and lets its parent know (using a signal for example).
 * The parent then looks at all of its children and updates them or itself.
 * Basically, we traverse the hierarchy upwards until the LayoutItem finds that its size did not change.
 * At that point, it updates all of its children.
 * They in turn also determine if their size actually changed or not and if it didn't they don't pass the call onwards.
 * There isn't any other way of a Widget to communicate upwards the hierarchy, except its size, is there...?
 * There is opacity, but that only works downwards.
 * Let's say there isn't until proven otherwise.
 * We could use signals, but I guess they are unnecessary in this setup as both children know their parents and parent
 * know their children explicitly.
 * Also, for using signals, I would have to implement a disconnect method ... not yet.
 * Okay, so what happens?
 * Some Widget changes its size.
 * This may happen because it was just created (size changed from zero), it was just removed (size changed to zero) or
 * actually because its size changed.
 * I guess creation, show, hide, deletion are points in time where I can automatically relayout.
 *
 * Okay, so we now have a virtual relayout() function that needs to report whether the size of the LayoutItem changed.
 * This leads to the question, how does a LayoutItem decide its size?
 * Well, it has several constraints.
 *
 ** The Items's claim.
 *  Consists of a horizontal and vertical Strech.
 *  Is a hard constraint that no parent should ever be able to change.
 *
 *
 *
 *
 * WTF?
 * Could it be, that Widgets are the leafs of the Widget hierarchy and may themselves never have children?
 *
 */
