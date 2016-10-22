#pragma once

#include <algorithm>
#include <limits>

#include "common/index.hpp"
#include "core/layout.hpp"

namespace notf {

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

class StackLayoutItem : public LayoutItem {

    friend class AbstractLayout;

public: // methods
    /// \brief Virtual Destructor.
    virtual ~StackLayoutItem() override;

    /// \brief Empty default Constructor.
    explicit StackLayoutItem() = default;

protected: // for StackLayout
    /// \brief Value Constructor.
    /// \param layout_object    The LayoutObject owned by this Item.
    explicit StackLayoutItem(std::shared_ptr<LayoutObject> layout_object)
        : LayoutItem(std::move(layout_object))
    {
    }
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

class StackLayout : public BaseLayout<StackLayoutItem> {

public: // enum
    /// \brief Direction in which the StackLayout is stacked.
    enum class DIRECTION : unsigned char {
        LEFT_TO_RIGHT,
        TOP_TO_BOTTOM,
        RIGHT_TO_LEFT,
        BOTTOM_TO_TOP,
    };

public: // methods
    /// \brief Direction in which the StackLayout is stacked.
    DIRECTION get_direction() const { return m_direction; }

    //    void set_direction();

    //    void set_spacing();

    /// \brief Finds the Index of the LayoutObject in this Layout, be be invalid.
    Index find_index(const Handle handle) const
    {
        for (Index::Type index = 0; index < m_items.size(); ++index) {
            if (handle == m_items[index]) {
                return Index(index);
            }
        }
        return Index::make_invalid();
    }

    /// \brief Returns the LayoutItem at a given Index in this Layout, be be invalid.
    StackLayoutItem& get_index(const Index index)
    {
        if (!index || index.get() >= m_items.size()) {
            return get_item(BAD_HANDLE);
        }
        return get_item(m_items[index.get()]);
    }

    /// \brief Places a new Widget into the Layout, and returns the current Widget.
    //    StackLayoutItem& add_item(std::shared_ptr<Widget> widget);

    /// \brief Looks for a Widget at a given local position.
    /// \param local_pos    Local coordinates where to look for the Widget.
    /// \return The Widget at a given local position or an empty shared_ptr if there is none.
    virtual std::shared_ptr<Widget> get_widget_at(const Vector2& local_pos) const override;

public: // static methods
    /// \brief Factory function to create a new FillLayout.
    /// \param direction    Direction in which the StackLayout is stacked.
    /// \param handle       Handle of this Layout.
    static std::shared_ptr<StackLayout> create(const DIRECTION direction, Handle handle = BAD_HANDLE)
    {
        return create_item<StackLayout>(handle, direction);
    }

protected: // methods
    /// \brief Value Constructor.
    /// \param handle   Handle of this Layout.
    explicit StackLayout(const Handle handle, const DIRECTION direction)
        : BaseLayout(handle)
        , m_direction(direction)
        , m_spacing(0)
        , m_items()
    {
    }

private: // fields
    /// \brief Direction in which the StackLayout is stacked.
    DIRECTION m_direction;

    /// \brief Spacing between the individual Items.
    Real m_spacing;

    /// \brief All items in this Layout in order.
    std::vector<Handle> m_items;
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

} // namespace notf