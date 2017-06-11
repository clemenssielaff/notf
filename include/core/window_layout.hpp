#pragma once

#include "core/layout.hpp"

namespace notf {

/**********************************************************************************************************************/

/** The WindowLayout is owned by a Window and root of all LayoutItems displayed within the Window.
 *
 */
class WindowLayout : public Layout {
    friend class Window;

protected: // factory *************************************************************************************************/
    /** @param window   Window owning this RootWidget. */
    WindowLayout(Window* window);

    static std::shared_ptr<WindowLayout> create(Window* window);

public: // methods ****************************************************************************************************/
    /** Find all Widgets at a given position in the Window.
     * @param local_pos     Local coordinates where to look for a Widget.
     * @return              All Widgets at the given coordinate, ordered from front to back.
     */
    std::vector<Widget*> get_widgets_at(const Vector2f& screen_pos);

    /** Sets a new Controller for the WindowLayout. */
    void set_controller(ControllerPtr controller);

private: // methods ***************************************************************************************************/
    virtual void _remove_child(const Item* child_item) override;

    virtual void _get_widgets_at(const Vector2f& local_pos, std::vector<Widget*>& result) const override;

    virtual Claim _consolidate_claim() override;

    virtual void _relayout() override;

private: // fields
    /** The Window Controller. */
    Controller* m_controller;
};

} // namespace notf
