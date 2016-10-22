#pragma once

#include <assert.h>
#include <memory>
#include <unordered_map>

#include "common/signal.hpp"
#include "common/size2r.hpp"
#include "common/transform2.hpp"
#include "core/abstract_item.hpp"

namespace notf {

class Layout;
class LayoutObject;
class LayoutRoot;
class Widget;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/// \brief Visibility states, all but one mean that the LayoutObject is not visible, but all for different reasons.
enum class VISIBILITY {
    INVISIBLE, // LayoutObject is not displayed
    HIDDEN, // LayoutObject is not INVISIBLE but one of its parents is, so it cannot be displayed
    UNROOTED, // LayoutObject and all ancestors are not INVISIBLE, but the Widget is not a child of a RootWidget
    VISIBLE, // LayoutObject is displayed
};

/// \brief Coordinate Spaces to pass to get_transform().
enum class SPACE {
    PARENT, // returns transform in local coordinates, relative to the parent LayoutObject
    WINDOW, // returns transform in global coordinates, relative to the Window
    SCREEN, // returns transform in screen coordinates, relative to the screen origin
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/*
 * \brief A LayoutObject is anything that can be put into a Layout - a Widget or any subclass of Layout.
 */
class LayoutObject : public AbstractItem {

public: // abstract methods
    /// \brief Looks for a Widget at a given local position.
    /// \param local_pos    Local coordinates where to look for the Widget.
    /// \return The Widget at a given local position or an empty shared_ptr if there is none.
    virtual std::shared_ptr<Widget> get_widget_at(const Vector2& local_pos) const = 0;

public: // methods
    /// \brief Virtual destructor.
    virtual ~LayoutObject() override;

    /// \brief Returns true iff this LayoutObject has a parent
    bool has_parent() const { return !m_parent.expired(); }

    /// \brief Tests if a given LayoutObject is a child of this LayoutObject.
    bool has_child(const std::shared_ptr<LayoutObject>& candidate) const;

    /// \brief Returns true iff this LayoutObject has at least one child.
    bool has_children() const { return !m_children.empty(); }

    /// \brief Tests, if this LayoutObject is a descendant of the given `ancestor`.
    /// \param ancestor Potential ancestor
    /// \return True iff `ancestor` is an ancestor of this LayoutObject, false otherwise.
    bool has_ancestor(const std::shared_ptr<LayoutObject>& ancestor) const;

    /// \brief Returns the parent LayoutObject containing this LayoutObject, may be invalid.
    std::shared_ptr<const LayoutObject> get_parent() const { return m_parent.lock(); }

    /// \brief Returns the LayoutObject of the hierarchy containing this LayoutObject.
    /// Is invalid if this LayoutObject is unrooted.
    std::shared_ptr<const LayoutRoot> get_root() const;

    /// \brief Checks the visibility of this LayoutObject.
    VISIBILITY get_visibility() const { return m_visibility; }

    /// \brief Returns the unscaled size of this LayoutObject in pixels.
    const Size2r& get_size() const { return m_size; }

    /// \brief Returns this LayoutObject's transformation in the given space.
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
    /// \brief Emitted when this LayoutObject got a new parent.
    /// \param Handle of the new parent.
    Signal<Handle> parent_changed;

    /// \brief Emitted when a new child LayoutObject was added to this one.
    /// \param Handle of the new child.
    Signal<Handle> child_added;

    /// \brief Emitted when a child LayoutObject of this one was removed.
    /// \param Handle of the removed child.
    Signal<Handle> child_removed;

    /// \brief Emitted, when the visibility of this LayoutObject has changed.
    /// \param New visiblity.
    Signal<VISIBILITY> visibility_changed;

    /// \brief Emitted when this LayoutObject' size changed.
    /// \param New size.
    Signal<Size2r> size_changed;

protected: // methods
    /// \brief Value Constructor.
    /// \param handle   Application-unique Handle of this Item.
    explicit LayoutObject(const Handle handle)
        : AbstractItem(handle)
        , m_parent()
        , m_children()
        , m_visibility(VISIBILITY::VISIBLE)
        , m_size()
        , m_transform(Transform2::identity())
    {
    }

    /// \brief Returns a child LayoutObject, is invalid if no child with the given Handle exists.
    std::shared_ptr<LayoutObject> get_child(const Handle child_handle) const;

    /// \brief Returns all children of this LayoutObject.
    const std::unordered_map<Handle, std::shared_ptr<LayoutObject>>& get_children() const { return m_children; }

    /// \brief Adds the given child to this LayoutObject.
    void add_child(std::shared_ptr<LayoutObject> child_object);

    /// \brief Removes the child with the given Handle.
    void remove_child(const Handle child_handle);

    /// \brief Shows (if possible) or hides this LayoutObject.
    void set_visible(const bool is_visible);

    /// \brief Updates the size of this LayoutObject.
    void set_size(const Size2r size)
    {
        m_size = size;
        size_changed(m_size);
    }

    /// \brief Tells the object and all of its children to redraw.
    virtual void redraw();

private: // methods
    /// \brief Sets a new LayoutObject to contain this LayoutObject.
    void set_parent(std::shared_ptr<LayoutObject> parent);

    /// \brief Removes the current parent of this LayoutObject.
    void unparent() { set_parent({}); }

    /// \brief Recursive function to let all children emit visibility_changed when the parent's visibility changed.
    void cascade_visibility(const VISIBILITY visibility);

    /// \brief Recursive implementation to produce the LayoutObject's transformation in window space.
    void get_window_transform(Transform2& result) const;

    /// \brief Returns the LayoutObject's transformation in screen space.
    Transform2 get_screen_transform() const;

    /// \brief Returns the LayoutObject's transformation in parent space.
    Transform2 get_parent_transform() const { return m_transform; }

private: // fields
    /// \brief The parent LayoutObject, may be invalid.
    std::weak_ptr<LayoutObject> m_parent;

    /// \brief All children of this LayoutObject.
    std::unordered_map<Handle, std::shared_ptr<LayoutObject>> m_children;

    /// \brief Visibility state of this LayoutObject.
    VISIBILITY m_visibility;

    /// \brief Unscaled size of this LayoutObject in pixels.
    Size2r m_size;

    /// \brief 2D transformation of this LayoutObject in local space.
    Transform2 m_transform;

    CALLBACKS(LayoutObject)
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

} // namespace notf

// TODO: redraw methods
// As is it set up right now, there is no strong relationship between a Widget and its child widgets, they can be
// positioned all over the place.
// Unlike in Qt, we cannot say that a widget must update all of its child widgets when it redraws but that is a good
// thing as you might not need that.
// Instead, every widget that changes should register with the Window's renderer.
// Just before rendering, the Renderer then figures out what Widgets to redraw, only consulting their bounding box
// overlaps and z-values, ignoring the Widget hierarchy.
