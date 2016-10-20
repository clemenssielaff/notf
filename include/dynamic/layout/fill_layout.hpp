#pragma once

#include "core/layout.hpp"

namespace signal {

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

class FillLayoutItem : public LayoutItem {

public: // methods
    virtual ~FillLayoutItem() override;

protected: // for LayoutRoot
    /// \brief Empty default Constructor.
    explicit FillLayoutItem() = default;

    /// \brief Value Constructor.
    /// \param layout_object    The LayoutObject owned by this Item.
    explicit FillLayoutItem(std::shared_ptr<LayoutObject> layout_object)
        : LayoutItem(std::move(layout_object))
    {
    }
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

class FillLayout : public BaseLayout<FillLayoutItem> {

public: // methods
    /// Checks, if this Layout contains a Widget.
    bool has_widget() const { return has_children(); }

    /// \brief Returns the Widget contained in this Layout.
    std::shared_ptr<Widget> get_widget() const;

    /// \brief Places a new Widget into the Layout, and returns the current Widget.
    std::shared_ptr<Widget> set_widget(std::shared_ptr<Widget> widget);

    /// \brief Looks for a Widget at a given local position.
    /// \param local_pos    Local coordinates where to look for the Widget.
    /// \return The Widget at a given local position or an empty shared_ptr if there is none.
    virtual std::shared_ptr<Widget> get_widget_at(const Vector2& local_pos) const override;

public: // static methods
    /// \brief Factory function to create a new FillLayout.
    /// \param handle   Handle of this Layout.
    static std::shared_ptr<FillLayout> create(Handle handle = BAD_HANDLE) { return create_item<FillLayout>(handle); }

protected: // methods
    /// \brief Value Constructor.
    /// \param handle   Handle of this Layout.
    explicit FillLayout(const Handle handle)
        : BaseLayout(handle)
    {
    }
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

} // namespace signal
