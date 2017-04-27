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

    private: // fields
        /** Scroll Area containing this ScrollBar. */
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

private: // fields
    /** Window into the content. */
    std::shared_ptr<Overlayout> m_area_window;

    /** Scrolled layout containing the ScrollArea's content. */
    std::shared_ptr<Overlayout> m_scroll_container;

    /** Vertical scroll bar. */
    std::shared_ptr<ScrollBar> m_vscrollbar;

    /** Controller providing the content of the scrolled area. */
    ControllerPtr m_content;
};

} // namespace notf
