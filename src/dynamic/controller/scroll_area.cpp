#include "dynamic/controller/scroll_area.hpp"

#include "common/log.hpp"
#include "common/xform2.hpp"
#include "core/events/mouse_event.hpp"
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

void ScrollArea::Background::_paint(Painter&) const
{
}

ScrollArea::ScrollArea()
    : BaseController<ScrollArea>({}, {})
    , m_area_window()
    , m_scroll_container()
    , m_vscrollbar()
    , m_content()
{
}

void ScrollArea::_initialize()
{
    m_area_window = Overlayout::create();
    m_area_window->set_claim(Claim());

    std::shared_ptr<Background> background = std::make_shared<Background>();
    m_area_window->add_item(background);
    background->on_scroll.connect([this](MouseEvent& event) -> void {
        if (m_content) {
            Xform2f content_transform = m_scroll_container->get_transform();
            m_scroll_container->set_transform(content_transform.translate({0, 16 * event.window_delta.y}));
        }
    });

    m_scroll_container = Overlayout::create();
    m_scroll_container->set_scissor(m_area_window);
    m_area_window->add_item(m_scroll_container);

    m_vscrollbar = std::make_shared<ScrollBar>(*this);

    std::shared_ptr<StackLayout> root_layout = StackLayout::create(StackLayout::Direction::LEFT_TO_RIGHT);
    root_layout->add_item(m_area_window);
    root_layout->add_item(m_vscrollbar);
    _set_root_item(root_layout);
}

void ScrollArea::set_area_controller(ControllerPtr controller)
{
    controller->initialize(); // TODO: maybe initialize when you add a controller somewhere?
    if (!controller->get_root_item()) {
        log_critical << "Cannot initialize Controller " << controller->get_id();
        return;
    }

    m_content = controller;

    m_scroll_container->clear();
    m_scroll_container->add_item(m_content);
}

} // namespace notf
