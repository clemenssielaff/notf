#include "dynamic/controller/scroll_area.hpp"

#include "dynamic/layout/overlayout.hpp"
#include "dynamic/layout/stack_layout.hpp"
#include "graphics/cell/painter.hpp"

namespace notf {

ScrollArea::ScrollBar::ScrollBar(ScrollArea& scroll_area)
    : m_scroll_area(scroll_area)
{
    Claim::Stretch horizontal, vertical;
    horizontal.set_fixed(10);
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
    , m_main_layout()
    , m_area_layout()
    , m_horizontal_layout()
    , m_vscrollbar()
{
}

void ScrollArea::_initialize()
{
    m_area_layout = Overlayout::create();

    m_vscrollbar = std::make_shared<ScrollBar>(*this);

    m_horizontal_layout = StackLayout::create(StackLayout::Direction::LEFT_TO_RIGHT);
    m_horizontal_layout->add_item(m_area_layout);
    m_horizontal_layout->add_item(m_vscrollbar);

    m_main_layout = Overlayout::create();
    _set_root_item(m_main_layout);                //
    m_main_layout->add_item(m_horizontal_layout); // TODO: does it still work if I switch the two lines here?
}

void ScrollArea::set_area_controller(std::shared_ptr<Controller> controller)
{
    m_area_layout->clear();
    m_area_layout->add_item(controller);
}

} // namespace notf
