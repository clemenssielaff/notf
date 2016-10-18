#pragma once

#include "core/old_layout.hpp"

namespace signal {

class Widget;

class FillLayout : public OldLayout {

public: // methods
    /// \brief Returns the Widget contained in this Layout.
    std::shared_ptr<Widget> get_widget() const;

    /// \brief Places a new Widget into the Layout, and returns the current Widget.
    std::shared_ptr<Widget> set_widget(std::shared_ptr<Widget> widget);

    /// \brief Looks for a Widget at a given local position.
    /// \param local_pos    Local coordinates where to look for the Widget.
    /// \return The Widget at a given local position or an empty shared_ptr if there is none.
    virtual std::shared_ptr<Widget> get_widget_at(const Vector2& local_pos) const override;

    /// \brief Lets the parent LayoutObjects know that the .
    virtual void redraw() override;

public: // static methods
    /// \brief Factory function to create a new FillLayout.
    /// \param handle   Handle of this Layout.
    static std::shared_ptr<FillLayout> create(Handle handle = BAD_HANDLE) { return create_item<FillLayout>(handle); }

protected: // methods
    /// \brief Value Constructor.
    /// \param handle   Handle of this Layout.
    explicit FillLayout(const Handle handle)
        : OldLayout(handle)
    {
    }
};

} // namespace signal
