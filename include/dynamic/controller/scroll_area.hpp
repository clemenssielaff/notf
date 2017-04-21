#pragma once

#include "core/controller.hpp"
#include "core/widget.hpp"

namespace notf {

class Overlayout;
class StackLayout;

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

public: // methods
    ScrollArea();

    virtual void _initialize() override;

    /** Displays the content of the given Controller inside the scroll area.  */
    void set_area_controller(std::shared_ptr<Controller> controller);

private: // fields
    /** Layout containing the scrolled Area. */
    std::shared_ptr<Overlayout> m_area_layout;

    /** Layout providing a split between the scrolled area and the ScrollBar widgetg. */
    std::shared_ptr<StackLayout> m_main_layout;

    /** Vertical scroll bar. */
    std::shared_ptr<ScrollBar> m_vscrollbar;
};

} // namespace notf
