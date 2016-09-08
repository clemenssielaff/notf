#pragma once

#include <memory>
#include <vector>

#include "common/transform2.hpp"
#include "core/component.hpp"

namespace signal {

class LayoutItem;
class Widget;

/// \brief Abstract base class for all Layouts Components.
///
class LayoutComponent : public Component {

public: // methods
    /// \brief This Component's type.
    virtual KIND get_kind() const override { return KIND::LAYOUT; }

    /// \brief Returns the Widget owning this Layout, can be empty if the Widget has expired.
    std::shared_ptr<Widget> get_widget() { return m_widget.lock(); }

    /// \brief Returns the LayoutItem at a given local position.
    /// \param local_pos    Local coordinates where to look for the LayoutItem.
    /// \return LayoutItem at the given position or an empty shared pointer, if there is none.
    virtual std::shared_ptr<LayoutItem> item_at(const Vector2& local_pos) = 0;

    /// \brief Updates the Layout.
    virtual void update() = 0;

protected: // methods
    /// \brief Value Constructor.
    explicit LayoutComponent(std::shared_ptr<Widget> owner);

private: // fields
    /// \brief Widget owning this Layout.
    std::weak_ptr<Widget> m_widget;
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/// \brief A non-owning, abstract container for Widgets in a Layout.
///
class LayoutItem {

public: // methods
    /// \brief Virtual Destructor.
    virtual ~LayoutItem() = default;

    /// no copy / assignment
    LayoutItem(const LayoutItem&) = delete;
    LayoutItem& operator=(const LayoutItem&) = delete;

    /// \brief Returns the Widget contained in this LayoutItem. Can be empty if the Widget has expired.
    std::shared_ptr<Widget> get_widget() const { return m_widget.lock(); }

    /// \brief Returns the 2D Transformation of this LayoutItem relative to its Layout.
    virtual Transform2 get_transform() const = 0;

protected: // methods
    /// \brief Value Constructor
    /// \param layout   Layout that this LayoutItem is a part of.
    /// \param widget   Widget contained in this LayoutItem.
    explicit LayoutItem(std::shared_ptr<LayoutComponent> layout, std::shared_ptr<Widget> widget);

protected: // fields
    /// \brief Layout that this LayoutItem is a part of.
    std::weak_ptr<LayoutComponent> m_layout;

    /// \brief Widget contained in this LayoutItem.
    std::weak_ptr<Widget> m_widget;
};

} // namespace signal
