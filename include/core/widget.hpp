#pragma once

#include "core/screen_item.hpp"

namespace notf {

class Cell;
class CellCanvas;
class Layout;
class MouseEvent;
class Painter;

/**********************************************************************************************************************/

/** A Widget is something drawn on screen that the user can interact with.
 * The term "Widget" is a mixture of "Window" and "Gadget".
 * Cells
 * =====
 * While the Widget determines the size and the state of what is drawn, the actual drawing is performed on "Cells".
 * A Widget can have multiple Cells.
 *
 * Scissoring
 * ==========
 * In order to implement scroll areas that contain a view on Widgets that are never drawn outside of its boundaries,
 * those Widgets need to be "scissored" by the scroll area.
 * A "Scissor" is an axis-aligned rectangle, scissoring is the act of cutting off parts of a Widget that fall outside
 * that rectangle and since this operation is provided by OpenGL, it is pretty cheap.
 * Every Widget contains a pointer to the parent Layout that acts as its scissor.
 * An empty pointer means that this Widget is scissored by its parent Layout, but every Layout in this Widget's Item
 * ancestry can be used (including the Windows WindowLayout, which effectively disables scissoring).
 */
class Widget : public ScreenItem {

protected: // Constructor
    /** Default Constructor. */
    explicit Widget();

public: // methods
    /** Destructor. */
    virtual ~Widget() override;

    /** Returns the Layout used to scissor this Widget.
     * Returns an empty shared_ptr, if no explicit scissor Layout was set or the scissor Layout has since expired.
     * In this case, the Widget is implicitly scissored by its parent Layout.
     */
    std::shared_ptr<Layout> get_scissor() const { return m_scissor_layout.lock(); }

    /** The Cell used to display the Widget on screen. */
    std::shared_ptr<Cell> get_cell() const { return m_cell; }

    /** Sets the new scissor Layout for this Widget.
     * @param scissor               New scissor Layout, must be an Layout in this Widget's Item hierarchy or empty.
     * @throw std::runtime_error    If the scissor is not an ancestor Layout of this Widget.
     */
    void set_scissor(std::shared_ptr<Layout> get_scissor);

    /** Sets a new Claim for this Widget.
     * @return  True, iff the currenct Claim was changed.
     */
    bool set_claim(const Claim get_claim);

    /** Tells the Render Manager that this Widget needs to be redrawn. */
    void redraw();

    /** Draws the Widget's Cell onto the screen.
     * Either uses the cached Cell or updates it first, using _paint().
     */
    void paint(CellCanvas &cell_context) const;

public: // signals
    /** Signal invoked when this Widget is asked to handle a Mouse move event. */
    Signal<MouseEvent&> on_mouse_move;

    /** Signal invoked when this Widget is asked to handle a Mouse button event. */
    Signal<MouseEvent&> on_mouse_button;

private: // methods
    /** Redraws the Cell with the Widget's current state. */
    virtual void _paint(Painter& painter) const = 0;

    virtual void _get_widgets_at(const Vector2f& local_pos, std::vector<Widget*>& result) const override;

private: // fields
    /** Reference to a Layout used to 'scissor' this Widget.
     * An expired weak_ptr is treated like an empty pointer.
     */
    std::weak_ptr<Layout> m_scissor_layout;

    /** Cell to draw this Widget into. */
    std::shared_ptr<Cell> m_cell;

    /** Clean Widgets can use their current Cell when painted, dirty Widgets have to redraw their Cell. */
    mutable bool m_is_clean;
};

} // namespace notf

/* Draw / Redraw procedure
 * =======================
 * A ScreenItem changes a Property
 * This results in the RenderManager getting a request to redraw the frame
 * The RenderManager then collects all Widgets, sorts them calls Widget::paint() on them
 * Widget::paint() checks if it needs to update its Cell
 *  if so, it creates a Painter object and calls the pure virtual function Widget::_paint(Painter&) which does the actual work
 *
 * State Machines
 * ==============
 * Maybe not all Widgets should have State Machines by default.
 * I think they are great in general, but they might just be better as an optional mixin-class, so you are not forced
 * to have a State Machine for even the most simplest Widget.
 */
