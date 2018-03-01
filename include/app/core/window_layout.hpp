#pragma once

#include "app/core/layout.hpp"

namespace notf {

//====================================================================================================================//

/// The WindowLayout is owned by a Window and root of all LayoutItems displayed within the Window.
class WindowLayout : public Layout {

    // types ---------------------------------------------------------------------------------------------------------//
public:
    /// Private access type template.
    /// Used for finer grained friend control and is compiled away completely (if you should worry).
    template<typename T, typename = typename std::enable_if<is_one_of<T, Window>::value>::type>
    class Private;

    // methods -------------------------------------------------------------------------------------------------------//
protected:
    /// Constructor.
    /// @param window   Window owning this RootWidget.
    WindowLayout(Window* window);

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
    /// The Window Controller.
    Controller* m_controller;
};

// ===================================================================================================================//

template<>
class WindowLayout::Private<Window> {
    friend class Window;

    /// Constructor.
    /// @param window_layout    WindowLayout to access.
    Private(WindowLayout& window_layout) : m_window_layout(window_layout) {}

    /// Factory.
    /// @param window   Window owning this RootWidget.
    static WindowLayoutPtr create(Window* window) { return WindowLayout::create(window); }

    /// Updates the Grant of this Item and might cause a relayout.
    /// @return      True iff the Grant was modified.
    bool set_grant(Size2f grant) { return m_window_layout._set_grant(std::move(grant)); }

    /// The WindowLayout to access.
    WindowLayout& m_window_layout;
};

} // namespace notf
