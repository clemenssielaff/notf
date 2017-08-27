#pragma once

#include "core/screen_item.hpp"

namespace notf {

class CellCanvas;
class Painter;

class Cell;
using CellPtr = std::shared_ptr<Cell>;

/**********************************************************************************************************************/

/** A Widget is something drawn on screen that the user can interact with.
 * The term "Widget" is a mixture of "Window" and "Gadget".
 *
 * Cells
 * =====
 * While the Widget determines the size and the state of what is drawn, the actual drawing is performed in a "Cell".
 * Multiple Widgets may share a Cell.
 *
 * Capabilities
 * ============
 * Sometimes Layouts need more information from a Widget than just its bounding rect in order to place it correctly.
 * For example, a TextLayout will try to align two subsequent widgets displaying text in a way that makes it look, like
 * both Widgets are part of the same continuous text.
 * This only works, if the TextLayout knows the font size and vertical baseline offset of each of the Widgets.
 * However, these are not fields that are available in the Widget baseclass -- nor should they be, since most other
 * Widgets do not display text in that way.
 * This information is therefore separate from the actual Widget, contained in a so-called Widget "Capability".
 * Any Widget that is capable of being displayed inline in a continuous text will have a certain Capability which can
 * be queried by the TextLayout and used to position the Widget correctly.
 * If a Widget does not have the requested Capability, it will throw an exception and it is the caller's job to catch
 * it and react appropriately.
 */
class Widget : public ScreenItem {

    friend class RenderManager; // can call _render()

protected: // constructor *********************************************************************************************/
    Widget();

public: // methods ****************************************************************************************************/
    /** The Cell used to display the Widget on screen. */
    CellPtr get_cell() const { return m_cell; }

    /** Sets a new Claim for this Widget.
     * @return True iff the Claim was modified.
     */
    bool set_claim(const Claim claim) { return _set_claim(std::move(claim)); }

    /** Tells the Render Manager that this Widget needs to be redrawn. */
    void redraw() const;

protected: // methods *************************************************************************************************/
    virtual void _relayout() override;

private: // methods ***************************************************************************************************/
    /** Renders the Widget's Cell onto the screen.
     * Is called only by the RenderManager.
     * Either uses the cached Cell or updates it first, using _paint().
     */
    void _render(CellCanvas& cell_context) const;

    /** Redraws the Cell with the Widget's current state. */
    virtual void _paint(Painter& painter) const = 0;

    virtual void _remove_child(const Item*) override { assert(0); } // should never happen

    virtual void _get_widgets_at(const Vector2f& local_pos, std::vector<Widget*>& result) const override;

    // hide Item methods that have no effect for Widgets
    using Item::has_child;
    using Item::has_children;

private: // fields ****************************************************************************************************/
    /** Cell to draw this Widget into. */
    CellPtr m_cell;

    /** Clean Widgets can use their current Cell when rendered, dirty Widgets have to redraw their Cell. */
    mutable bool m_is_clean;
};

} // namespace notf
