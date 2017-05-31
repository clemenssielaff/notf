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
    bool set_claim(const Claim claim);

    /** Tells the Render Manager that this Widget needs to be redrawn. */
    void redraw() const;

private: // methods ***************************************************************************************************/
    /** Renders the Widget's Cell onto the screen.
     * Is called only by the RenderManager.
     * Either uses the cached Cell or updates it first, using _paint().
     */
    void _render(CellCanvas& cell_context) const;

    /** Redraws the Cell with the Widget's current state. */
    virtual void _paint(Painter& painter) const = 0;

    virtual Aabrf _get_content_aabr() const override { return Aabrf(_get_size()); };

    virtual void _remove_child(const Item*) override { assert(0); }

    virtual void _get_widgets_at(const Vector2f& local_pos, std::vector<Widget*>& result) const override;

    virtual bool _set_size(const Size2f size) override { return ScreenItem::_set_size(std::move(size)); }

    // hide Item methods that have no effect for Widgets
    using Item::has_child;
    using Item::has_children;

private: // fields
    /** Cell to draw this Widget into. */
    CellPtr m_cell;

    /** Clean Widgets can use their current Cell when rendered, dirty Widgets have to redraw their Cell. */
    mutable bool m_is_clean;
};

} // namespace notf
