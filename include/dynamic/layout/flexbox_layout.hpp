#include "core/components/layout_component.hpp"

namespace signal {

class FlexboxLayoutItem;

/// \brief Layout to arrange Widgets inside a flexbox.
///
class FlexboxLayoutComponent : public LayoutComponent {

public: // enum
    /// \brief Direction of the Flexbox.
    enum class DIRECTION {
        INVALID = 0,
        RIGHT,
        LEFT,
        UP,
        DOWN,
    };

public: // methods
    /// \brief Sets the direction into which the Flexbox expands.
    /// If wrap direction == direction, this will also set the wrap direction to keep the non-wrapping behaviour.
    void set_direction(DIRECTION direction)
    {
        if(m_direction == m_wrap_direction){
            m_wrap_direction = direction;
        }
        m_direction = direction;
        update();
    }

    /// \brief Sets the direction into which the Flexbox wraps.
    /// If wrap direction == direction, the flexbox will not wrap.
    void set_wrap_direction(DIRECTION direction)
    {
        m_wrap_direction = direction;
        update();
    }

    /// \brief Adds a new Widget into the Layout.
    /// \param widget   Widget to add.
    /// \return The created Layout Item.
    std::weak_ptr<FlexboxLayoutItem> add_widget(std::shared_ptr<Widget> widget);

    /// \brief Returns the LayoutItem at a given local position.
    /// \param local_pos    Local coordinates where to look for the LayoutItem.
    /// \return LayoutItem at the given position or an empty shared pointer, if there is none.
    virtual std::shared_ptr<LayoutItem> item_at(const Vector2& local_pos) override;

    /// \brief Updates the layout.
    virtual void update() override;

protected: // methods
    /// \brief Value Constructor.
    /// \param owner            Widget owning this Layout.
    /// \param direction        Primary direction of the Flexbox.
    /// \param wrap_direction   Secondary direction of the Flexbox, defaults to the direction parameter (non-wrapping).
    explicit FlexboxLayoutComponent(std::shared_ptr<Widget> owner, DIRECTION direction,
                                    DIRECTION wrap_direction = DIRECTION::INVALID);

protected: // fields
    /// \brief All Layout items contained in this widget.
    std::vector<std::shared_ptr<FlexboxLayoutItem>> m_items;

    /// \brief Primary direction of this Flexbox (where new items are added, if there is enough space).
    DIRECTION m_direction;

    /// \brief Secondary direction of this Flexbox, where overflowing items are wrapped.
    DIRECTION m_wrap_direction;
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/// \brief Layout items in a Flexbox Layout
///
class FlexboxLayoutItem : public LayoutItem {

    friend class FlexboxLayoutComponent;

public: // methods
    /// \brief Returns the 2D Transformation of this LayoutItem relative to its Layout.
    virtual Transform2 get_transform() const override;

    /// \brief Collapsed Flexbox LayoutItems only contribute their size only to the layout's secondary axis.
    void set_collapsed(const bool collapsed)
    {
        if(collapsed != m_is_collapsed){
            m_is_collapsed = collapsed;
            m_layout.lock()->update(); // TODO: plain pointers from LayoutItem to Layout?
        }
    }

private: // methods for FlexboxLayoutComponent
    /// \brief Value Constructor
    /// \param layout   Layout that this LayoutItem is a part of.
    /// \param widget   Widget contained in this LayoutItem.
    explicit FlexboxLayoutItem(std::shared_ptr<FlexboxLayoutComponent> layout, std::shared_ptr<Widget> widget);

protected: // fields

    bool m_is_collapsed;
};

} // namespace signal

// TODO: continue here by creating a size component? or is that a part of the shape?
