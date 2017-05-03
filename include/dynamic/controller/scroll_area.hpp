#pragma once

#include "core/controller.hpp"
#include "core/widget.hpp"

namespace notf {

class Overlayout;
class ScrollArea;
class StackLayout;

using ScrollAreaPtr = std::shared_ptr<ScrollArea>;

/** Scroll Area Controller. */
class ScrollArea : public BaseController<ScrollArea> {

private: // types
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

    class Background : public Widget {
    public: // methods
        /** Constructor. */
        Background()
            : Widget() {}

        virtual void _paint(Painter& painter) const override;
    };

public: // methods
    ScrollArea();

    virtual void _initialize() override;

    /** Displays the content of the given Controller inside the scroll area.  */
    void set_area_controller(ControllerPtr controller);

private: // methods
    /** Updates the position and size of the scrollbar. */
    void _update_scrollbar(float delta_y);

    /** Calculates the height of the ScrollArea's content. */
    float _get_content_height() const;

private: // fields
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
