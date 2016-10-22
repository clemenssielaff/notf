#pragma once

#include "core/layout.hpp"

namespace notf {

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

class LayoutRootItem : public LayoutItem {

protected: // for LayoutRoot
    /// \brief Empty default Constructor.
    explicit LayoutRootItem() = default;

    /// \brief Value Constructor.
    /// \param layout_object    The LayoutObject owned by this Item.
    explicit LayoutRootItem(std::shared_ptr<LayoutObject> layout_object)
        : LayoutItem(std::move(layout_object))
    {
    }
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/*
 * \brief Root Layout owned by a Window and root of all LayoutObjects displayed within the Window.
 */
class LayoutRoot : public BaseLayout<LayoutRootItem> {

    friend class Window;

public: // methods
    /// \brief Returns the Window owning this LayoutRoot.
    std::shared_ptr<Window> get_window() const { return m_window.lock(); }

    /// \brief Looks for a Widget at a given local position.
    /// \param local_pos    Local coordinates where to look for the Widget.
    /// \return The Widget at a given local position or an empty shared_ptr if there is none.
    virtual std::shared_ptr<Widget> get_widget_at(const Vector2& local_pos) const override;

    /// Checks, if there is a Layout contained in this root LayoutObject.
    bool has_layout() const { return has_children(); }

    /// \brief Changes the internal Layout of the LayoutRoot.
    void set_layout(std::shared_ptr<AbstractLayout> item);

protected: // methods
    /// \brief Value Constructor.
    /// \param handle   Handle of this Widget.
    /// \param window   Window owning this RootWidget.
    explicit LayoutRoot(Handle handle, std::shared_ptr<Window> window)
        : BaseLayout(handle)
        , m_window(window)
    {
    }

private: // methods
    /// \brief Returns the Layout contained in this LayoutRoot, may be invalid.
    std::shared_ptr<AbstractLayout> get_layout() const;

private: // static methods for Window
    /// \brief Factory function to create a new LayoutRoot.
    /// \param handle   Handle of this LayoutRoot.
    /// \param window   Window owning this LayoutRoot.
    static std::shared_ptr<LayoutRoot> create(Handle handle, std::shared_ptr<Window> window)
    {
        return create_item<LayoutRoot>(handle, std::move(window));
    }

private: // fields
    /// \brief The Window containing this LayoutRoot.
    const std::weak_ptr<Window> m_window;
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

} // namespace notf
