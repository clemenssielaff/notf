#pragma once

#include "app/scene/widget/layout.hpp"

NOTF_OPEN_NAMESPACE

//====================================================================================================================//

/// The WindowLayout is owned by a Window and root of all LayoutItems displayed within the Window.
class WindowLayout : public Layout {

    // types ---------------------------------------------------------------------------------------------------------//
public:
    NOTF_ACCESS_TYPES(Window)

    // methods -------------------------------------------------------------------------------------------------------//
protected:
    /// Constructor.
    /// @param window   Window owning this RootWidget.
    WindowLayout(Window* window);

    /// The Window containing the hierarchy that this Item is a part of.
    /// Is invalid if this Item is not part of a rooted hierarchy.
    Window* window() const { return m_window; }

    /// Factory.
    /// @param window   Window owning this RootWidget.
    static WindowLayoutPtr create(Window* window);

public:
    /// Destructor.
    virtual ~WindowLayout() override {}

    /// Find all Widgets at a given position in the Window.
    /// @param local_pos     Local coordinates where to look for a Widget.
    /// @return              All Widgets at the given coordinate, ordered from front to back.
    std::vector<Widget*> widgets_at(const Vector2f& screen_pos);

    /// Sets a new Controller for the WindowLayout.
    void set_controller(const ControllerPtr& controller);

private:
    virtual void _remove_child(const Item* child_item) override;

    virtual void _widgets_at(const Vector2f& local_pos, std::vector<Widget*>& result) const override;

    virtual Claim _consolidate_claim() override;

    virtual void _relayout() override;

    // fields --------------------------------------------------------------------------------------------------------//
private:
    /// The Window containing the hierarchy that this Item is the root of.
    Window* m_window;

    /// The Window Controller.
    Controller* m_controller;
};

// ===================================================================================================================//

template<>
class WindowLayout::Access<Window> {
    friend class Window;

    /// Constructor.
    /// @param window_layout    WindowLayout to access.
    Access(WindowLayout& window_layout) : m_window_layout(window_layout) {}

    /// Factory.
    /// @param window   Window owning this RootWidget.
    static WindowLayoutPtr create(Window* window) { return WindowLayout::create(window); }

    /// Updates the Grant of this Item and might cause a relayout.
    /// @return      True iff the Grant was modified.
    bool set_grant(Size2f grant) { return m_window_layout._set_grant(std::move(grant)); }

    /// The WindowLayout to access.
    WindowLayout& m_window_layout;
};

NOTF_CLOSE_NAMESPACE
