#pragma once

#include <vector>

#include "common/size2r.hpp"
#include "core/abstract_item.hpp"

namespace signal {

class LayoutObject;
class Vector2;
class Widget;
class LayoutRoot;

class AbstractLayoutObject : public AbstractItem {

    friend class LayoutObject;

public: // methods
    /// \brief Virtual destructor.
    virtual ~AbstractLayoutObject() override;

    /// \brief Returns true iff this LayoutObject has a parent
    bool has_parent() const { return !m_parent.expired(); }

    /// \brief Returns the parent LayoutObject, may be invalid.
    std::shared_ptr<AbstractLayoutObject> get_parent() const { return m_parent.lock(); }

    /// \brief Returns the RootWidget of the hierarchy containing this LayoutObject.
    /// Is invalid if this LayoutObject is unrooted.
    std::shared_ptr<const LayoutRoot> get_root_item() const;

    /// \brief Tests, if `ancestor` is an ancestor of this LayoutObject.
    /// \param ancestor Potential ancestor
    /// \return True iff `ancestor` is an ancestor of this LayoutObject, false otherwise.
    bool is_ancestor_of(const std::shared_ptr<AbstractLayoutObject> ancestor);

    /// \brief Tests if a given LayoutObject is the internal child of this LayoutObject.
    bool has_internal_child(const LayoutObject* const layout_object) const
    {
        return (m_internal_child && layout_object == m_internal_child.get());
    }

    /// \brief Tests if a given LayoutObject is an external child of this LayoutObject.
    bool has_external_child(const LayoutObject* const layout_object) const
    {
        for (const std::shared_ptr<LayoutObject>& external_child : m_external_children) {
            if (layout_object == external_child.get()) {
                return true;
            }
        }
        return false;
    }

    /// \brief Tests if a given LayoutObject is a child of this LayoutObject.
    bool has_child(const LayoutObject* const layout_object) const
    {
        return has_internal_child(layout_object) || has_external_child(layout_object);
    }

    /// \brief Returns the unscaled size of this LayoutObject in pixels.
    virtual Size2r get_size() const = 0;

    /// \brief Looks for a Widget at a given local position.
    /// \param local_pos    Local coordinates where to look for the Widget.
    /// \return The Widget at a given local position or an empty shared_ptr if there is none.
    virtual std::shared_ptr<Widget> get_widget_at(const Vector2& local_pos) const = 0;

    /// \brief Tells the containing layout to redraw (potentially cascading up the Widget ancestry).
    virtual void redraw() = 0;

protected: // methods
    /// \brief Value Constructor.
    /// \param handle   Application-unique Handle of this LayoutObject.
    explicit AbstractLayoutObject(const Handle handle)
        : AbstractItem(handle)
        , m_parent()
        , m_internal_child()
        , m_external_children()
    {
    }

    /// \brief Returns the internal child or an empty shared pointer if there isn't one.
    const std::shared_ptr<LayoutObject>& get_internal_child() const { return m_internal_child; }

    /// \brief Returns all external children.
    const std::vector<std::shared_ptr<LayoutObject>>& get_external_children() const { return m_external_children; }

    /// \brief Sets a new parent Object.
    /// If the parent is already a child of this Object, the operation is ignored and returns false.
    /// \param parent   New parent Object.
    /// \return True iff the parent was changed successfully.
    virtual bool set_parent(std::shared_ptr<AbstractLayoutObject> parent);

    /// \brief Unroots this LayoutObject by clearing its parent.
    void unparent() { set_parent({}); }

    /// \brief Sets the internal child of this LayoutObject, an existing internal child is dropped.
    /// \param child    New internal child.
    void set_internal_child(std::shared_ptr<LayoutObject> child);

    /// \brief Removes a child LayoutObject.
    /// \param parent   Child LayoutObject to remove.
    void remove_child(std::shared_ptr<LayoutObject> child);

protected: // methods for LayoutObject
    /// \brief Updates the internal
//    virtual void update_layout() = 0;

private: // fields
    /// \brief Parent of this LayoutObject.
    std::weak_ptr<AbstractLayoutObject> m_parent;

    /// \brief The internal child LayoutObject, may be invalid.
    std::shared_ptr<LayoutObject> m_internal_child;

    /// \brief All external children.
    std::vector<std::shared_ptr<LayoutObject>> m_external_children;
};

} // namespace signal

// TODO: redraw methods
// As is it set up right now, there is no strong relationship between a Widget and its child widgets, they can be
// positioned all over the place.
// Unlike in Qt, we cannot say that a widget must update all of its child widgets when it redraws but that is a good
// thing as you might not need that.
// Instead, every widget that changes should register with the Window's renderer.
// Just before rendering, the Renderer then figures out what Widgets to redraw, only consulting their bounding box
// overlaps and z-values, ignoring the Widget hierarchy.
