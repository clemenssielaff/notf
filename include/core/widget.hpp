#pragma once

#include "core/screen_item.hpp"

namespace notf {

class Cell;
class CellCanvas;
class Layout;
class Painter;

/**********************************************************************************************************************/

/** A Widget is something drawn on screen that the user can interact with.
 * The term "Widget" is a mixture of "Window" and "Gadget".
 *
 * Cells
 * =====
 * While the Widget determines the size and the state of what is drawn, the actual drawing is performed in a "Cell".
 * Multiple Widgets may share a Cell.
 *
 * Events
 * ======
 * Only Widgets receive events, which means that in order to handle events, a Layout must contain an invisible Widget
 * in the background (see ScrollArea for an example).
 */
class Widget : public ScreenItem {

protected: // Constructor
    /** Default Constructor. */
    Widget(); // TODO: there is no save factory for Controller and Widgets

public: // methods
    /** Destructor. */
    virtual ~Widget() override;

    /** The Cell used to display the Widget on screen. */
    std::shared_ptr<Cell> get_cell() const { return m_cell; }

    /** Sets a new Claim for this Widget.
     * @return True iff the Claim was modified.
     */
    bool set_claim(const Claim get_claim);

    /** Tells the Render Manager that this Widget needs to be redrawn. */
    void redraw();

    /** Draws the Widget's Cell onto the screen.
     * Either uses the cached Cell or updates it first, using _paint().
     */
    void paint(CellCanvas& cell_context) const;

private: // methods
    /** Redraws the Cell with the Widget's current state. */
    virtual void _paint(Painter& painter) const = 0;

    virtual void _get_widgets_at(const Vector2f& local_pos, std::vector<Widget*>& result) const override;

    virtual void _set_render_layer(const RenderLayerPtr& render_layer) override;

private: // fields
    /** Cell to draw this Widget into. */
    std::shared_ptr<Cell> m_cell;

    /** Clean Widgets can use their current Cell when painted, dirty Widgets have to redraw their Cell. */
    mutable bool m_is_clean;
};

} // namespace notf
