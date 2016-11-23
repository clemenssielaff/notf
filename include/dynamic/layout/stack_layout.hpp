#pragma once

#include <algorithm>
#include <limits>

#include "common/index.hpp"
#include "common/padding.hpp"
#include "core/layout.hpp"

namespace notf {

/**
 * @brief The StackLayout class
 */
class StackLayout : public Layout {

public: // methods
    /** Direction in which items are stacked. */
    Direction get_direction() const { return m_direction; }

    /** Alignment of items in the main direction. */
    Alignment get_alignment() const { return m_main_alignment; }

    /** Alignment of items in the cross direction. */
    Alignment get_cross_alignment() const { return m_cross_alignment; }

    /** How items are wrapped. */
    Wrap get_wrapping() const { return m_wrap; }

    /** Spacing between items. */
    float get_spacing() const { return m_spacing; }

    /** Padding around the Layout's border. */
    const Padding& get_padding() const { return m_padding; }

    /** Defines the direction in which the StackLayout is stacked. */
    void set_direction(const Direction direction);

    /** Defines the alignment of items in the main direction. */
    void set_alignment(const Alignment alignment);

    /** Defines the alignment of items in the cross direction. */
    void set_cross_alignment(const Alignment alignment);

    /** Defines how items are wrapped. */
    void set_wrapping(const Wrap) {}

    /** Defines the spacing between items. */
    void set_spacing(float spacing);

    /** Defines the padding around the Layout's border. */
    void set_padding(const Padding& padding);

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
    static std::shared_ptr<StackLayout> create(const Direction direction, Handle handle = BAD_HANDLE)
    {
        return _create_object<StackLayout>(handle, direction);
    }

protected: // methods
    /// @brief Value Constructor.
    /// @param handle   Handle of this Layout.
    explicit StackLayout(const Handle handle, const Direction direction)
        : Layout(handle)
        , m_direction(direction)
        , m_main_alignment(Alignment::START)
        , m_cross_alignment(Alignment::START)
        , m_wrap(Wrap::NONE)
        , m_spacing(0.f)
        , m_padding(Padding::none())
        , m_items()
    {
    }

    virtual void _update_claim() override;

    virtual void _remove_item(const Handle item_handle) override;

    virtual void _relayout(const Size2f total_size) override;

private: // methods
    /** Calculates the cross offset to accomodate the cross alignment constraint. */
    float cross_align_offset(const float item_size, const float available_size);

private: // fields
    /** Direction in which the StackLayout is stacked. */
    Direction m_direction;

    /** Alignment of items in the main direction. */
    Alignment m_main_alignment;

    /** Alignment of items in the cross direction. */
    Alignment m_cross_alignment;

    /** How items in the Layout are wrapped. */
    Wrap m_wrap;

    /** Spacing between items in the layout*/
    float m_spacing;

    /** Padding around the Layout's borders. */
    Padding m_padding;

    /// @brief All items in this Layout in order.
    std::vector<Handle> m_items;
};

} // namespace notf
