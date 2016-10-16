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

#include "common/signal.hpp"
#include "common/size_range.hpp"
#include "common/transform2.hpp"
#include "core/abstract_layout_item.hpp"

namespace signal {

class RootLayoutItem;

/// \brief Abstraction layer for something that can be put into a Layout - a Widget or another Layout.
class LayoutItem : public AbstractLayoutItem {

public: // enums
    /// \brief Coordinate Spaces to pass to get_transform().
    enum class SPACE {
        PARENT, // returns transform in local coordinates, relative to the parent LayoutItem
        WINDOW, // returns transform in global coordinates, relative to the Window
        SCREEN, // returns transform in screen coordinates, relative to the screen origin
    };

    /// \brief Visibility states, all but one mean that the LayoutItem is not visible, but all for different reasons.
    enum class VISIBILITY {
        INVISIBLE, // LayoutItem is not displayed
        HIDDEN, // LayoutItem is not INVISIBLE but one of its parents is, so it cannot be displayed
        UNROOTED, // LayoutItem and all ancestors are not INVISIBLE, but the Widget is not a child of a RootWidget
        VISIBLE, // LayoutItem is displayed
    };

public: // methods
    /// \brief Virtual destructor.
    virtual ~LayoutItem() override;

    /// \brief Checks the visibility of this LayoutItem.
    VISIBILITY get_visibility() const { return m_visibility; }

    /// \brief Shows (if possible) or hides this LayoutItem.
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

public: // signals
    /// \brief Emitted, when the visibility of this LayoutItem has changed.
    /// \param New visiblity.
    Signal<VISIBILITY> visibility_changed;

    /// \brief Emitted, when the LayoutItem is about to be destroyed.
    Signal<> about_to_be_destroyed;

protected: // methods
    /// \brief Value Constructor.
    explicit LayoutItem(const Handle handle)
        : AbstractLayoutItem(handle)
        , m_visibility(VISIBILITY::VISIBLE)
        , m_transform(Transform2::identity())
        , m_horizontal_size()
        , m_vertical_size()
    {
    }

    /// \brief Sets a new parent Item.
    /// If the parent is already a child of this Item, the operation is ignored and returns false.
    /// \param parent   New parent Item.
    /// \return True iff the parent was changed successfully.
    virtual bool set_parent(std::shared_ptr<AbstractLayoutItem> parent) override;

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
    void update_visibility(const VISIBILITY visibility);

protected: // fields
    /// \brief Visibility state of this LayoutItem.
    VISIBILITY m_visibility;

    /// \brief 2D transformation of this LayoutItem in local space.
    Transform2 m_transform;

    /// \brief Horizontal size range of the LayoutItem.
    SizeRange m_horizontal_size;

    /// \brief Vertical size range of the LayoutItem.
    SizeRange m_vertical_size;

    CALLBACKS(LayoutItem)
};

} // namespace signal
