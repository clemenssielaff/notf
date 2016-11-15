#pragma once

#include <algorithm>
#include <limits>

#include "common/const.hpp"
#include "common/index.hpp"
#include "core/layout.hpp"

namespace notf {

/**
 * @brief The StackLayout class
 */
class StackLayout : public Layout {

public: // methods
    /// @brief Direction in which the StackLayout is stacked.
    STACK_DIRECTION get_direction() const { return m_direction; }

    /// @brief Updates the direction in which the StackLayout is stacked.
    void set_direction(const STACK_DIRECTION direction)
    {
        if (m_direction == direction) {
            return;
        }
        m_direction = direction;
        _update_claim();
        if (_is_dirty()) {
            _update_parent_layout();
        }
    }

    //    void set_spacing();

    /// @brief Finds the Index of the LayoutItem in this Layout, might be invalid.
    Index find_index(const Handle handle) const
    {
        for (Index::Type index = 0; index < m_items.size(); ++index) {
            if (handle == m_items[index]) {
                return Index(index);
            }
        }
        return Index::make_invalid();
    }

    /// @brief Returns the LayoutItem at a given Index in this Layout, might be invalid.
    std::shared_ptr<LayoutItem> get_index(const Index index)
    {
        if (!index || index.get() >= m_items.size()) {
            return {};
        }
        return _get_child(m_items[index.get()]);
    }

    /// @brief Adds a new LayoutItem into the Layout.
    /// @param item     Item to place at the end of the Layout. If it is already a child, it is moved to last place.
    void add_item(std::shared_ptr<LayoutItem> item);

    virtual std::shared_ptr<Widget> get_widget_at(const Vector2& local_pos) override;

public: // static methods
    /// @brief Factory function to create a new StackLayout.
    /// @param direction    Direction in which the StackLayout is stacked.
    /// @param handle       Handle of this Layout.
    static std::shared_ptr<StackLayout> create(const STACK_DIRECTION direction, Handle handle = BAD_HANDLE)
    {
        return _create_object<StackLayout>(handle, direction);
    }

protected: // methods
    /// @brief Value Constructor.
    /// @param handle   Handle of this Layout.
    explicit StackLayout(const Handle handle, const STACK_DIRECTION direction)
        : Layout(handle)
        , m_direction(direction)
        , m_items()
    {
    }

    virtual void _update_claim() override;

    virtual void _remove_item(const Handle item_handle) override;

    virtual void _relayout(const Size2r size) override;

private: // fields
    /// @brief Direction in which the StackLayout is stacked.
    STACK_DIRECTION m_direction;

    float m_spacing;

    /// @brief All items in this Layout in order.
    std::vector<Handle> m_items;
};

} // namespace notf
