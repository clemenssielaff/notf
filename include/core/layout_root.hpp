#pragma once

#include "core/layout.hpp"

namespace notf {

/*
 * @brief Root Layout owned by a Window and root of all LayoutItems displayed within the Window.
 */
class LayoutRoot : public Layout {

    friend class Window;

public: // methods
    /// @brief Returns the Window owning this LayoutRoot.
    std::shared_ptr<Window> get_window() const { return m_window.lock(); }

    virtual std::shared_ptr<Widget> get_widget_at(const Vector2& local_pos) override;

    /// Checks, if there is a Layout contained in this root LayoutItem.
    bool has_layout() const { return !is_empty(); }

    /// @brief Changes the internal Layout of the LayoutRoot.
    void set_layout(std::shared_ptr<Layout> item);

protected: // methods
    /// @brief Value Constructor.
    /// @param handle   Handle of this Widget.
    /// @param window   Window owning this RootWidget.
    explicit LayoutRoot(Handle handle, std::shared_ptr<Window> window)
        : Layout(handle)
        , m_window(window)
    {
    }

private: // methods
    /// @brief Returns the Layout contained in this LayoutRoot, may be invalid.
    std::shared_ptr<Layout> get_layout() const;

private: // static methods for Window
    /// @brief Factory function to create a new LayoutRoot.
    /// @param handle   Handle of this LayoutRoot.
    /// @param window   Window owning this LayoutRoot.
    static std::shared_ptr<LayoutRoot> create(Handle handle, std::shared_ptr<Window> window)
    {
        return create_item<LayoutRoot>(handle, std::move(window));
    }

private: // fields
    /// @brief The Window containing this LayoutRoot.
    const std::weak_ptr<Window> m_window;
};

} // namespace notf
