#pragma once

#include <type_traits>

#include "core/layout_item.hpp"

namespace notf {

/**
 * @brief Abstract Layout baseclass.
 */
class Layout : public LayoutItem {

    friend class LayoutItem;

public: // methods
    /// @brief Virtual Destructor
    virtual ~Layout() override;

    /// @brief Tests if a given LayoutItem is a child of this LayoutItem.
    bool has_child(const std::shared_ptr<LayoutItem>& candidate) const;

    /// @brief Returns the number of items in this Layout.
    size_t count_children() const { return get_children().size(); }

    /// @brief Checks if this Layout is empty.
    bool is_empty() const { return m_children.empty(); }

    using LayoutItem::set_visible;

public: // signals
    /// @brief Emitted when a new child LayoutItem was added to this one.
    /// @param Handle of the new child.
    Signal<Handle> child_added;

    /// @brief Emitted when a child LayoutItem of this one was removed.
    /// @param Handle of the removed child.
    Signal<Handle> child_removed;

protected: // methods
    /// @brief Value Constructor.
    /// @param handle   Application-unique Handle of this Item.
    explicit Layout(const Handle handle)
        : LayoutItem(handle)
    {
    }

    /// @brief Returns a child LayoutItem, is invalid if no child with the given Handle exists.
    std::shared_ptr<LayoutItem> get_child(const Handle child_handle) const;

    /// @brief Returns all children of this LayoutItem.
    const std::unordered_map<Handle, std::shared_ptr<LayoutItem>>& get_children() const { return m_children; }

    /// @brief Adds the given child to this LayoutItem.
    void add_child(std::shared_ptr<LayoutItem> child_object);

    /// @brief Removes the child with the given Handle.
    void remove_child(const Handle child_handle);

    /// @brief Tells this LayoutItem to update its size based on the size of its children and its parents restrictions.
    //    virtual bool relayout() = 0;
    virtual bool relayout() { return false; }

    /// @brief Recursively propagates a layout change downwards to all descendants of this LayoutItem.
    /// Recursion is stopped, when a LayoutItem didn't need to change its size.
    void relayout_down();

    virtual void cascade_visibility(const VISIBILITY visibility) override;

    virtual void redraw() override;

private: // fields
    /// @brief All children of this Layout.
    std::unordered_map<Handle, std::shared_ptr<LayoutItem>> m_children;
};

} // namespace notf
