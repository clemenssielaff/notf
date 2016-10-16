#pragma once

#include <memory>

#include "core/abstract_layout_item.hpp"

namespace signal {

class Layout;

/*
 * \brief LayoutItem owned by a Window and root of all LayoutItems displayed within the Window.
 */
class RootLayoutItem : public AbstractLayoutItem {

    friend class Window;

public: // methods
    /// \brief Returns the Window owning this RootWidget.
    std::shared_ptr<Window> get_window() const { return m_window.lock(); }

    /// \brief Looks for a Widget at a given local position.
    /// \param local_pos    Local coordinates where to look for the Widget.
    /// \return The Widget at a given local position or an empty shared_ptr if there is none.
    virtual std::shared_ptr<Widget> get_widget_at(const Vector2& local_pos) const override;

    /// \brief Changes the internal Layout of the RootLayoutItem.
    void set_layout(std::shared_ptr<Layout> item);

    /// \brief Draws the internal child and all of its descendants recursively.
    virtual void redraw() override;

protected: // methods
    /// \brief Value Constructor.
    /// \param handle   Handle of this Widget.
    /// \param window   Window owning this RootWidget.
    explicit RootLayoutItem(Handle handle, std::shared_ptr<Window> window)
        : AbstractLayoutItem(handle)
        , m_window(window)
    {
    }

private: // static methods for Window
    /// \brief Factory function to create a new RootWidget.
    /// \param handle   Handle of this Widget.
    /// \param window   Window owning this RootWidget.
    static std::shared_ptr<RootLayoutItem> create(Handle handle, std::shared_ptr<Window> window)
    {
        return create_item<RootLayoutItem>(handle, std::move(window));
    }

private: // fields
    /// \brief The Window containing this RootWidget.
    const std::weak_ptr<Window> m_window;
};

} // namespace signal
