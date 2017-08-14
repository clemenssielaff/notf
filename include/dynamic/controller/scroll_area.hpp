#pragma once

#include "core/controller.hpp"
#include "core/widget.hpp"

namespace notf {

class Overlayout;
class ScrollArea;
class FlexLayout;

using ScrollAreaPtr = std::shared_ptr<ScrollArea>;

//*********************************************************************************************************************/

/** Scroll Area Controller.
 * Consists of multiple layouts and a ScrollBar widget.
 * The ScrollArea has no claim of its own and will use whatever space it is granted by the parent layout.
 *
 *  The basic setup is as follows:
 *
 * +- RootLayout ------------------------+---+
 * | +- AreaWindow --------------------+ | S |
 * | | +- ScrollContainer -----------+ | | c |
 * | | |                             | | | r |
 * | | |                             | | | o |
 * | | |                             | | | l |
 * | | |                             | | | l |
 * | | |                             | | | B |
 * | | |                             | | | a |
 * | +-:-----------------------------:-+ | r |
 * +---:-----------------------------:---+---+
 *     |                             |
 *     +-----------------------------+
 *
 * The ScrollArea controller has a FlexLayout at its root, the *RootLayout*.
 * Its only job is to place the Scrollbar widget next to the area used to view the scrolled content.
 *
 * The FlexLayout contains an Overlayout, the *AreaWindow*, which fullfills two roles:
 * 1. It has an explicit non-claim, meaning that it will ignore the claims of its children and make use only of the
 *    space granted by the RootLayout.
 *    This way, we completely decouple the ScrollArea from its displayed content.
 * 2. To act as a scissor for the content.
 *
 * The deepest nested item, the *ScrollContainer* is another Overlayout used to move the content around within the
 * AreaWindow.
 * It doesn't supply its own claim, meaning that it will fit itself tightly around whatever is displayed within.
 *
 * Stacked behind the ScrollContainer, in the AreaWindow is another widget, the invisible *Background* widget.
 * Its job is to catch mouse wheel events that went unhandled by the ScrollArea's content.
 */
class ScrollArea : public BaseController<ScrollArea> {

private: // types  ***************************************************************************************************/
    class ScrollBar : public Widget {

    public: // methods
        /** Constructor. */
        ScrollBar(ScrollArea& scroll_area);

        virtual void _paint(Painter& painter) const override;

    public: // fields
        /** Size of the actual bar in relation to the height of the ScrollBar widget, in the range (0 -> 1].*/
        float size;

        /** Position of the actual bar in relation to the height of the ScrollBar widget, in the range [0 -> 1).*/
        float pos;

    private: // fields
        /** ScrollArea containing this ScrollBar. */
        ScrollArea& m_scroll_area;
    };

    // ****************************************************************************************************************/

    class Background : public Widget {
    public: // methods
        /** Constructor. */
        Background()
            : Widget() {}

        virtual void _paint(Painter& painter) const override;
    };

public: // methods ****************************************************************************************************/
    /** Constructor. */
    ScrollArea();

    /** Controller of the content inside the scroll area. */
    ControllerPtr get_area_controller() const { return m_content; }

    /** Displays the content of the given Controller inside the scroll area.  */
    void set_area_controller(ControllerPtr controller);

private: // methods  **************************************************************************************************/
    /** Updates the position and size of the scrollbar. */
    void _update_scrollbar(float delta_y);

    /** Calculates the height of the ScrollArea's content. */
    float _get_content_height() const;

private: // fields  ***************************************************************************************************/
    /** Window into the content. */
    std::shared_ptr<Overlayout> m_area_window;

    /** Scrolled layout containing the ScrollArea's content. */
    std::shared_ptr<Overlayout> m_scroll_container;

    /** Vertical scroll bar. */
    std::shared_ptr<ScrollBar> m_vscrollbar;

    /** Controller providing the content of the scrolled area. */
    ControllerPtr m_content;

    /** Connection enabled to drag the scrollbar with the cursor. */
    Connection m_on_scrollbar_drag;
};

} // namespace notf
