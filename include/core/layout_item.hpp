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
 */

#include <memory>
#include <vector>

#include "common/handle.hpp"
#include "common/transform2.hpp"

namespace signal {

class SizeRange;
struct Vector2;
class Widget;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/// \brief Abstraction layer for something that can be put into a Layout - a Widget or another Layout.
class LayoutItem {

public: // enums
    /// \brief Coordinate Spaces to pass to get_transform().
    enum class SPACE {
        PARENT, // returns transform in local coordinates, relative to the parent LayoutItem
        WINDOW, // returns transform in global coordinates, relative to the Window
        SCREEN, // returns transform in screen coordinates, relative to the screen origin
    };

public: // methods
    /// no copy / assignment
    LayoutItem(const LayoutItem&) = delete;
    LayoutItem& operator=(const LayoutItem&) = delete;

    /// \brief Virtual destructor.
    virtual ~LayoutItem();

    /// \brief The Application-unique Handle of this LayoutItem.
    Handle get_handle() const { return m_handle; }

    /// \brief Returns the Handle of the parent LayoutItem.
    /// The returned Handle may be BAD_HANDLE and the validity of the Handle cannot be guaranteed.
    Handle get_parent_handle() const;

    /// \brief Returns true iff this LayoutItem is visible, false if it is hidden.
    virtual bool is_visible() const = 0;

    /// \brief Returns this Widget's transformation in the given space.
    Transform2 get_transform(const SPACE space) const;

    /// \brief Looks for a Widget at a given local position.
    /// \param local_pos    Local coordinates where to look for the Widget.
    /// \return The Widget at a given local position or an empty shared_ptr if there is none.
    virtual std::shared_ptr<Widget> get_widget_at(const Vector2& local_pos) const = 0;

    /// \brief Returns the horizontal size range of this LayoutItem.
    virtual const SizeRange& get_horizontal_size() const = 0;

    /// \brief Returns the vertical size range of this LayoutItem.
    virtual const SizeRange& get_vertical_size() const = 0;

    /// \brief Tells the containing layout to redraw (potentially cascading up the Widget ancestry).
    virtual void redraw() = 0;

protected: // methods
    /// \brief Value Constructor.
    explicit LayoutItem(Handle handle)
        : m_handle(handle)
        , m_transform(Transform2::identity())
        , m_internal_child()
        , m_external_children()
    {
    }

    /// \brief Returns the internal child or an empty shared pointer if there isn't one.
    std::shared_ptr<LayoutItem> get_internal_child() const { return m_internal_child; }

    /// \brief Returns all external children.
    const std::vector<std::shared_ptr<LayoutItem>>& get_external_children() const { return m_external_children; }

    /// \brief Sets a new parent LayoutItem.
    /// \param parent   New parent LayoutItem.
    void set_parent(std::shared_ptr<LayoutItem> parent);

    /// \brief Removes a child LayoutItem.
    /// \param parent   Child LayoutItem to remove.
    void remove_child(std::shared_ptr<LayoutItem> child);

private: // fields
    /// \brief Application-unique Handle.
    const Handle m_handle;

    /// \brief 2D transformation of this LayoutItem in local space.
    Transform2 m_transform;

    /// \brief The internal child item, may be invalid.
    std::shared_ptr<LayoutItem> m_internal_child;

    /// \brief All external children.
    std::vector<std::shared_ptr<LayoutItem>> m_external_children;
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

} // namespace signal
