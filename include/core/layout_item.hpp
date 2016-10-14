#pragma once

/*
 * How to layout.
 *
 * Layouts and Widgets work in close collaboration.
 * The Widget hierarchy consists of alternating levels of Widgets and Layouts, with the root Widget at the top followed
 * by a single, internal Layout (see below), followed by one or more first-level Widgets each of which providing 0-n
 * child Layouts containing more Widgets (or Layouts) etc.
 * Each Window has its own hierarchy with a single root-Widget at the top.
 * LayoutItems can be moved within the hierarchy, store its Handle to to keep a persistent reference to a LayoutItem.
 * Using the Handle, you can even serialize and deserialize a hierarchy (which wouldn't work with pointers).
 *
 * Since the two concepts are intimately intertwined, Widgets and Layouts both derive from a virtual mixin-class
 * "LayoutItem", that provides the containing Layout information about the LayoutItem's preferred display parameters.
 *
 * A LayoutItem may have an internal and 0-n external Layouts.
 * The internal Layout is use to control the size of the LayoutItem from within.
 * Additionally, a LayoutItem may also have 0-n external Layouts, that are used to provide relative positions for
 * child LayoutItem but that do not influence its size like the internal Layout does.
 * External Layouts may also be empty, in which case they are simply ignored.
 *
 * Because each Widget must be contained in the Layout of its parent, a Widget doesn't reference its parent Widget
 * directly, but the Layout that it is contained in.
 * The Layout in turn knows the Widget that it belongs to.
 *
 * Visiblity.
 * Every LayoutItem has a visibility flag, denoting it as either visible or hidden - visible being the default state.
 * Visible LayoutItems are displayed and take up space in their parent Layout, while hidden LayoutItems are not
 * displayed and do not take up space in their parent Layout.
 * Children of an hidden LayoutItem are also considered hidden, even though they might have their visibility flag set.
 *
 */

#include <memory>
#include <unordered_map>
#include <vector>

#include "common/handle.hpp"
#include "common/signal.hpp"
#include "common/size_range.hpp"
#include "common/transform2.hpp"

namespace signal {

class SizeRange;
struct Vector2;
class Widget;
class WindowWidget;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/// \brief Abstraction layer for something that can be put into a Layout - a Widget or another Layout.
class LayoutItem : public std::enable_shared_from_this<LayoutItem> {

public: // enums
    /// \brief Coordinate Spaces to pass to get_transform().
    enum class SPACE {
        PARENT, // returns transform in local coordinates, relative to the parent LayoutItem
        WINDOW, // returns transform in global coordinates, relative to the Window
        SCREEN, // returns transform in screen coordinates, relative to the screen origin
    };

    enum class VISIBILITY {
        VISIBLE, // LayoutItem is displayed
        HIDDEN, // LayoutItem is VISIBLE but one of its parents is INVISIBLE, so it is not displayed
        UNROOTED, // LayoutItem is not the child of a RootWidget and cannot be displayed
        INVISIBLE, // LayoutItem is not displayed
    };

public: // methods
    /// no copy / assignment
    LayoutItem(const LayoutItem&) = delete;
    LayoutItem& operator=(const LayoutItem&) = delete;

    /// \brief Virtual destructor.
    virtual ~LayoutItem();

    /// \brief The Application-unique Handle of this LayoutItem.
    Handle get_handle() const { return m_handle; }

    /// \brief Returns the parent LayoutItem, may be invalid.
    std::shared_ptr<LayoutItem> get_parent() const { return m_parent.lock(); }

    /// \brief Tests, if 'ancestor' is an ancestor of this LayoutItem.
    /// \param ancestor Potential ancestor
    /// \return True iff ancestor is an ancestor, false otherwise.
    bool is_ancestor_of(const std::shared_ptr<LayoutItem> ancestor);

    /// \brief Returns the WindowWidget of the hierarchy containing this LayoutItem.
    const std::shared_ptr<WindowWidget> get_window_widget() const;

    /// \brief Checks the visibility of this LayoutItem.
    VISIBILITY get_visibility() const;

    /// \brief Returns true only if this LayoutItem is currently being displayed.
    /// See `get_visibility` for details on other visibility states.
    bool is_visible() const { return get_visibility() == VISIBILITY::VISIBLE; }

    /// \brief Shows or hides this LayoutItem.
    void set_visible(const bool is_visible);

    /// \brief Returns this LayoutItem's transformation in the given space.
    Transform2 get_transform(const SPACE space) const
    {
        switch (space) {
        case SPACE::WINDOW:
            return get_window_transform();

        case SPACE::SCREEN:
            return get_screen_transform();

        case SPACE::PARENT:
            break;
        }
        return get_parent_transform();
    }

    /// \brief Returns the horizontal size range of this LayoutItem.
    const SizeRange& get_horizontal_size() const { return m_horizontal_size; }

    /// \brief Returns the vertical size range of this LayoutItem.
    const SizeRange& get_vertical_size() const { return m_vertical_size; }

    /// \brief Looks for a Widget at a given local position.
    /// \param local_pos    Local coordinates where to look for the Widget.
    /// \return The Widget at a given local position or an empty shared_ptr if there is none.
    virtual std::shared_ptr<Widget> get_widget_at(const Vector2& local_pos) const = 0;

    /// \brief Tells the containing layout to redraw (potentially cascading up the Widget ancestry).
    virtual void redraw() = 0;

public: // signals
    /// \brief Emitted, when the visibility of this LayoutItem has changed.
    /// \param New visiblity.
    Signal<VISIBILITY> visibility_changed;

    /// \brief Emitted, when the LayoutItem is about to be destroyed.
    Signal<> about_to_be_destroyed;

protected: // methods
    /// \brief Value Constructor.
    explicit LayoutItem(Handle handle)
        : m_handle(handle)
        , m_parent()
        , m_is_visible(true)
        , m_transform(Transform2::identity())
        , m_horizontal_size()
        , m_vertical_size()
        , m_internal_child()
        , m_external_children()
    {
    }

    /// \brief Returns the internal child or an empty shared pointer if there isn't one.
    std::shared_ptr<LayoutItem> get_internal_child() const;

    /// \brief Returns all external children.
    const std::vector<std::shared_ptr<LayoutItem>>& get_external_children() const;

    /// \brief Sets a new parent LayoutItem.
    /// \param parent   New parent LayoutItem.
    void set_parent(std::shared_ptr<LayoutItem> parent);

    /// \brief Removes a child LayoutItem.
    /// \param parent   Child LayoutItem to remove.
    void remove_child(std::shared_ptr<LayoutItem> child);

private: // methods
    /// \brief Returns the LayoutItem's transformation in window space.
    Transform2 get_window_transform() const
    {
        Transform2 result = Transform2::identity();
        get_window_transform_impl(result);
        return result;
    }

    /// \brief Recursion implementation for get_window_transform().
    void get_window_transform_impl(Transform2& result) const;

    /// \brief Returns the LayoutItem's transformation in screen space.
    Transform2 get_screen_transform() const;

    /// \brief Returns the LayoutItem's transformation in parent space.
    Transform2 get_parent_transform() const { return m_transform; }

    /// \brief Recursive function to let all children emit visibility_changed when the parent's visibility changed.
    void emit_visibility_change_downstream(bool is_parent_visible) const;

protected: // fields
    /// \brief Application-unique Handle.
    const Handle m_handle;

    /// \brief Parent of this LayoutItem.
    std::weak_ptr<LayoutItem> m_parent;

    /// \brief Whether this LayoutItem is visible or hidden.
    bool m_is_visible;

    /// \brief 2D transformation of this LayoutItem in local space.
    Transform2 m_transform;

    /// \brief Horizontal size range of the LayoutItem.
    SizeRange m_horizontal_size;

    /// \brief Vertical size range of the LayoutItem.
    SizeRange m_vertical_size;

    /// \brief The internal child item, may be invalid.
    std::shared_ptr<LayoutItem> m_internal_child;

    /// \brief All external children.
    std::vector<std::shared_ptr<LayoutItem>> m_external_children;

    CALLBACKS(LayoutItem)
};

} // namespace signal
