#include "core/components/layout_component.hpp"

namespace signal {

/// \brief Layout to arrange Widgets inside a flexbox.
///
class FlexboxLayout : public LayoutComponent {

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
    /// \brief Appends a new Widget to the end of the Flexbox.
    void add_widget(std::shared_ptr<Widget> widget);

    /// \brief Returns the LayoutItem at a given local position.
    /// \param local_pos    Local coordinates where to look for the LayoutItem.
    /// \return LayoutItem at the given position or an empty shared pointer, if there is none.
    virtual std::shared_ptr<Widget> widget_at(const Vector2& local_pos) override;

    /// \brief Removes a given Widget from this Layout.
    virtual void remove_widget(std::shared_ptr<Widget> widget) override;

    /// \brief Sets the direction into which the Flexbox expands.
    /// If wrap direction == direction, this will also set the wrap direction to keep the non-wrapping behaviour.
    void set_direction(DIRECTION direction)
    {
        if (m_direction == m_wrap_direction) {
            m_wrap_direction = direction;
        }
        m_direction = direction;
//        update();
    }

    /// \brief Sets the direction into which the Flexbox wraps.
    /// If wrap direction == direction, the flexbox will not wrap.
    void set_wrap_direction(DIRECTION direction)
    {
        m_wrap_direction = direction;
//        update();
    }


protected: // methods
    /// \brief Value Constructor.
    /// \param owner            Widget owning this Layout.
    /// \param direction        Primary direction of the Flexbox.
    /// \param wrap_direction   Secondary direction of the Flexbox, defaults to the direction parameter (non-wrapping).
    explicit FlexboxLayout(std::shared_ptr<Widget> owner,
                           DIRECTION direction, DIRECTION wrap_direction = DIRECTION::INVALID);

protected: // fields
    /// \brief All Layout items contained in this widget.
    std::vector<std::shared_ptr<Widget>> m_items;

    /// \brief Primary direction of this Flexbox (where new items are added, if there is enough space).
    DIRECTION m_direction;

    /// \brief Secondary direction of this Flexbox, where overflowing items are wrapped.
    DIRECTION m_wrap_direction;
};

} // namespace signal
