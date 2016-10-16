#pragma once

#include <vector>

#include "core/abstract_item.hpp"

namespace signal {

class LayoutItem;
class Vector2;
class Widget;
class RootLayoutItem;

class AbstractLayoutItem : public AbstractItem {

public: // methods
    /// \brief Virtual destructor.
    virtual ~AbstractLayoutItem() override;

    /// \brief Returns true iff this LayoutItem has a parent
    bool has_parent() const { return !m_parent.expired(); }

    /// \brief Returns the parent LayoutItem, may be invalid.
    std::shared_ptr<AbstractLayoutItem> get_parent() const { return m_parent.lock(); }

    /// \brief Returns the RootWidget of the hierarchy containing this LayoutItem.
    /// Is invalid if this LayoutItem is unrooted.
    std::shared_ptr<const RootLayoutItem> get_root_item() const;

    /// \brief Tests, if `ancestor` is an ancestor of this LayoutItem.
    /// \param ancestor Potential ancestor
    /// \return True iff `ancestor` is an ancestor of this LayoutItem, false otherwise.
    bool is_ancestor_of(const std::shared_ptr<AbstractLayoutItem> ancestor);

    /// \brief Looks for a Widget at a given local position.
    /// \param local_pos    Local coordinates where to look for the Widget.
    /// \return The Widget at a given local position or an empty shared_ptr if there is none.
    virtual std::shared_ptr<Widget> get_widget_at(const Vector2& local_pos) const = 0;

    /// \brief Tells the containing layout to redraw (potentially cascading up the Widget ancestry).
    virtual void redraw() = 0;

protected: // methods
    /// \brief Value Constructor.
    /// \param handle   Application-unique Handle of this LayoutItem.
    explicit AbstractLayoutItem(const Handle handle)
        : AbstractItem(handle)
        , m_parent()
        , m_internal_child()
        , m_external_children()
    {
    }

    /// \brief Returns the internal child or an empty shared pointer if there isn't one.
    const std::shared_ptr<LayoutItem>& get_internal_child() const { return m_internal_child; }

    /// \brief Returns all external children.
    const std::vector<std::shared_ptr<LayoutItem>>& get_external_children() const { return m_external_children; }

    /// \brief Sets a new parent Item.
    /// If the parent is already a child of this Item, the operation is ignored and returns false.
    /// \param parent   New parent Item.
    /// \return True iff the parent was changed successfully.
    virtual bool set_parent(std::shared_ptr<AbstractLayoutItem> parent);

    /// \brief Unroots this LayoutItem by clearing its parent.
    void unparent() { set_parent({}); }

    /// \brief Sets the internal child of this LayoutItem, an existing internal child is dropped.
    /// \param child    New internal child.
    void set_internal_child(std::shared_ptr<LayoutItem> child); // TODO: special (thin) Layout class so that Widgets may only contain Layouts

    /// \brief Removes a child LayoutItem.
    /// \param parent   Child LayoutItem to remove.
    void remove_child(std::shared_ptr<LayoutItem> child);

private: // fields
    /// \brief Parent of this LayoutItem.
    std::weak_ptr<AbstractLayoutItem> m_parent;

    /// \brief The internal child LayoutItem, may be invalid.
    std::shared_ptr<LayoutItem> m_internal_child;

    /// \brief All external children.
    std::vector<std::shared_ptr<LayoutItem>> m_external_children;
};

} // namespace signal
