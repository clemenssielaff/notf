#pragma once

#include "core/layout.hpp"

namespace notf {

class FillLayout : public Layout {

public: // methods
    /// Checks, if this Layout contains a Widget.
    bool has_widget() const { return !is_empty(); }

    /// @brief Returns the Widget contained in this Layout.
    std::shared_ptr<Widget> get_widget() const;

    /// @brief Places a new Widget into the Layout, and returns the current Widget.
    std::shared_ptr<Widget> set_widget(std::shared_ptr<Widget> widget);

    virtual std::shared_ptr<Widget> get_widget_at(const Vector2& local_pos) override;

public: // static methods
    /// @brief Factory function to create a new FillLayout.
    /// @param handle   Handle of this Layout.
    static std::shared_ptr<FillLayout> create(Handle handle = BAD_HANDLE) { return create_item<FillLayout>(handle); }

protected: // methods
    /// @brief Value Constructor.
    /// @param handle   Handle of this Layout.
    explicit FillLayout(const Handle handle)
        : Layout(handle)
    {
    }

    /// @brief FillLayouts do not relayout.
    virtual bool relayout() override { return false; }
};

} // namespace notf
