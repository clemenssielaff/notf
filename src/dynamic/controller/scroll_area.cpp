#include "dynamic/controller/scroll_area.hpp"

#include "common/log.hpp"
#include "dynamic/layout/overlayout.hpp"
#include "dynamic/layout/stack_layout.hpp"
#include "graphics/cell/painter.hpp"

namespace notf {

ScrollArea::ScrollBar::ScrollBar(ScrollArea& scroll_area)
    : m_scroll_area(scroll_area)
{
    Claim::Stretch horizontal, vertical;
    horizontal.set_fixed(10);
    horizontal.set_priority(1);
    set_claim({horizontal, vertical});
}

void ScrollArea::ScrollBar::_paint(Painter& painter) const
{
    const Aabrf widget_rect = Aabrf(get_size());
    painter.begin_path();
    painter.add_rect(widget_rect);
    painter.set_fill_paint(Color("#454950"));
    painter.fill();
}

ScrollArea::ScrollArea()
    : BaseController<ScrollArea>({}, {})
    , m_area_layout()
    , m_main_layout()
    , m_vscrollbar()
{
}

void ScrollArea::_initialize()
{
    m_area_layout = Overlayout::create();
    m_area_layout->set_claim(Claim());

    m_vscrollbar = std::make_shared<ScrollBar>(*this);

    m_main_layout = StackLayout::create(StackLayout::Direction::LEFT_TO_RIGHT);
    m_main_layout->add_item(m_area_layout);
    m_main_layout->add_item(m_vscrollbar);
    _set_root_item(m_main_layout);
}

void ScrollArea::set_area_controller(ControllerPtr controller)
{
    controller->initialize(); // TODO: maybe initialize when you add a controller somewhere?
    if (!controller->get_root_item()) {
        log_critical << "Cannot initialize Controller " << controller->get_id();
        return;
    }

    m_area_layout->clear();
    m_area_layout->add_item(controller);
}

} // namespace notf
