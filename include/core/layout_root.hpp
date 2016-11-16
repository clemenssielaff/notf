#pragma once

#include "core/layout.hpp"

namespace notf {

/*
 * @brief The Layout Root is owned by a Window and root of all LayoutItems displayed within the Window.
 */
class LayoutRoot : public Layout {

    friend class Window;

public: // methods
    /// @brief Returns the Window owning this LayoutRoot.
    std::shared_ptr<Window> get_window() const { return m_window.lock(); }

    /// @brief Sets a new Item at the LayoutRoot.
    void set_item(std::shared_ptr<LayoutItem> item);

    virtual std::shared_ptr<Widget> get_widget_at(const Vector2& local_pos) override;

    /// @brief Updates the LayoutRoot after the Window's size has changed.
    void relayout(const Size2f size) { _relayout(std::move(size)); }

protected: // methods
    /// @brief Value Constructor.
    /// @param handle   Handle of this Widget.
    /// @param window   Window owning this RootWidget.
    explicit LayoutRoot(Handle handle, std::shared_ptr<Window> window)
        : Layout(handle)
        , m_window(window)
    {
    }

    virtual void _update_claim() override {}

    virtual void _remove_item(const Handle) override {}

    virtual void _relayout(const Size2f size) override;

private: // methods
    /// @brief Returns the Layout contained in this LayoutRoot, may be invalid.
    std::shared_ptr<LayoutItem> _get_item() const;

private: // static methods for Window
    /// @brief Factory function to create a new LayoutRoot.
    /// @param handle   Handle of this LayoutRoot.
    /// @param window   Window owning this LayoutRoot.
    static std::shared_ptr<LayoutRoot> create(Handle handle, std::shared_ptr<Window> window)
    {
        return _create_object<LayoutRoot>(handle, std::move(window));
    }

private: // fields
    /// @brief The Window containing this LayoutRoot.
    const std::weak_ptr<Window> m_window;
};

} // namespace notf
