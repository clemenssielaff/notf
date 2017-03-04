#pragma once

#include "core/layout_item.hpp"
#include "graphics/cell.hpp"
#include "utils/binding_accessors.hpp"

namespace notf {

template <typename T>
class MakeSmartEnabler;

class Painter;
class RenderContext;

/**********************************************************************************************************************/

/// @brief Something drawn on the screen, potentially able to interact with the user.
class Widget : public LayoutItem {

    friend class MakeSmartEnabler<Widget>;
    friend class RenderManager;

public: // methods
    virtual bool get_widgets_at(const Vector2f local_pos, std::vector<Widget*>& result) override;

    /** Returns the Layout used to scissor this Widget.
     * Returns an empty shared_ptr, if no explicit scissor Layout was set or the scissor Layout has since expired.
     * In this case, the Widget is implicitly scissored by its parent Layout.
     */
    std::shared_ptr<Layout> get_scissor() const { return m_scissor_layout.lock(); }

    /** Sets the new scissor Layout for this Widget. */
    void set_scissor(const std::shared_ptr<Layout>& scissor) { m_scissor_layout = scissor; }

    /** Sets a new Claim for this Widget.
     * @return  True, iff the currenct Claim was changed.
     */
    bool set_claim(const Claim claim);

    /** Tells the Window that its contents need to be redrawn. */
    void redraw()
    {
        m_cell.set_dirty();
        _redraw();
    }

    // clang-format off
protected_except_for_bindings: // methods for MakeSmartEnabler<Widget>;
    explicit Widget();

protected_except_for_bindings: // methods for RenderManager
    /** Draws the Widget's Cell onto the screen.
     * Either uses the cached Cell or updates it first, using _paint().
     */
    void paint(RenderContext &context) const;

    /** Redraws the Cell with the Widget's current state. */
    virtual void _paint(Painter& painter) const = 0;

    // clang-format on
private: // fields
    /** Reference to a Layout used to 'scissor' this item.
     * Example: a scroll area 'scissors' children that overflow its size.
     * An empty pointer means that this item is scissored by its parent Layout.
     * An expired weak_ptr is treated like an empty pointer.
     */
    std::weak_ptr<Layout> m_scissor_layout;

    /** Cell to draw this Widget into. */
    mutable Cell m_cell;
};

} // namespace notf

/* Draw / Redraw procedure:
 * A LayoutItem changes a Property
 * This results in the RenderManager getting a request to redraw the frame
 * The RenderManager then collects all Widgets, sorts them calls Widget::paint() on them
 * Widget::paint() checks if it needs to update its Cell
 *  if so, it creates a Painter object and calls the pure virtual function Widget::_paint(Painter&) which does the actual work
 *
 */
