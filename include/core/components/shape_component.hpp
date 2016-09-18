#pragma once

#include "common/aabr.hpp"
#include "common/size_range.hpp"
#include "core/component.hpp"

namespace signal {

class Widget;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/// \brief Virtual base class for all Shape Components.
///
class ShapeComponent : public Component {

public: // methods
    /// \brief This Component's type.
    virtual KIND get_kind() const override { return KIND::SHAPE; }

    /// \brief Returns the Shape's Axis Aligned Bounding Rect in screen coordinates.
    /// \param widget   Widget determining the shape.
    virtual Aabr get_screen_aabr(const Widget& widget) = 0;

    /// \brief Returns the vertical size range of this Shape.
    const SizeRange& get_vertical_size() const { return m_vertical_size; }

    /// \brief Returns the horizontal size range of this Shape.
    const SizeRange& get_horizontal_size() const { return m_horizontal_size; }

public: // signals
    /// \brief Emitted, when the horizontal size of this Shape changed.
    /// \param New size range.
    Signal<const SizeRange&> horizontal_size_changed;

    /// \brief Emitted, when the vertical size of this Shape changed.
    /// \param New size range.
    Signal<const SizeRange&> vertical_size_changed;

protected: // methods
    /// \brief Default Constructor.
    explicit ShapeComponent() = default;

    /// \brief Sets the horizontal size range of this Shape.
    void set_horizontal_size(const SizeRange& size)
    {
        m_horizontal_size = size;
        horizontal_size_changed(m_horizontal_size);
    }

    /// \brief Sets the vertical size range of this Shape.
    void set_vertical_size(const SizeRange& size)
    {
        m_vertical_size = size;
        vertical_size_changed(m_vertical_size);
    }

private: // fields
    /// \brief Vertical size range.
    SizeRange m_vertical_size;

    /// \brief Horizontal size range.
    SizeRange m_horizontal_size;
};

} // namespace signal
