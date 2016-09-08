#pragma once

#include <memory>
#include <vector>

#include "common/vector2.hpp"

namespace signal {

class LayoutItem;
class Widget;

/// \brief Abstract baseclass for all Layouts.
///
class Layout {

public: // methods
    /// \brief Default Constructor.
    explicit Layout() = default;

    /// no copy / assignment
    Layout(const Layout&) = delete;
    Layout& operator=(const Layout&) = delete;

    std::weak_ptr<LayoutItem> add_widget(std::shared_ptr<Widget> widget)
    {
        std::shared_ptr<LayoutItem> layout_item = std::make_shared<LayoutItem>(widget);
        std::weak_ptr<LayoutItem> result = layout_item;
        m_items.emplace_back(std::move(layout_item));
        return result;
    }

    /// \brief Returns the Widget at a given local position.
    /// \param local_pos    Local coordinates where to look for the Widget.
    /// \return Widget at the given position or an empty shared pointer, if there is none.
    ///
    virtual std::shared_ptr<Widget> widget_at(const Vector2& local_pos) = 0;

private: // fields
    /// \brief Widget owning this Layout.
    std::weak_ptr<Widget> m_widget;

    /// \brief All Layout items contained in this widget, accessible by index.
    std::vector<std::shared_ptr<LayoutItem>> m_items;
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/// \brief A non-owning container for Widgets in a Layout.
///
class LayoutItem {

public: // methods
    /// \brief Value Constructor.
    /// \param widget   Widget to contain in this LayoutItem.
    explicit LayoutItem(std::shared_ptr<Widget> widget)
        : m_widget(widget)
        , m_position(Vector2::fill(0))
        , m_rotation(0)
        , m_scale(Vector2::fill(1))
    {
    }

    /// no copy / assignment
    LayoutItem(const LayoutItem&) = delete;
    LayoutItem& operator=(const LayoutItem&) = delete;

    /// \brief Returns the Widget contained in this LayoutItem. Can be empty if the Widget expired.
    std::shared_ptr<Widget> get_widget() const { return m_widget.lock(); }

    /// \brief Returns this Widget's position in the parent's coordinate space.
    const Vector2& get_position() const { return m_position; }

    /// \brief Sets a new position of this Widget.
    void set_position(const Vector2& position);

    /// \brief Returns this Widget's rotation in the parent's coordinate space in radians.
    Real get_rotation() const { return m_rotation; }

    /// \brief Sets a new rotation of this Widget.
    void set_rotation(Real rotation);

    /// \brief Returns this Widget's scale vector.
    const Vector2& get_scale() const { return m_scale; }

    /// \brief Scales this Widget.
    void set_scale(const Vector2& scale);

private: // fields
    /// \brief Widget contained in this LayoutItem.
    std::weak_ptr<Widget> m_widget;

    /// \brief Layout that this LayoutItem is a part of.
    std::weak_ptr<Layout> m_layout;

    /// \brief Unscaled position relative to the parent Widget.
    Vector2 m_position;

    /// \brief Counter-clockwise rotation against the parent Widget in radians.
    Real m_rotation;

    /// \brief 2D scale factors.
    Vector2 m_scale;
};

} // namespace signal
