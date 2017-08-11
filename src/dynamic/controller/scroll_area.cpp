#include "dynamic/controller/scroll_area.hpp"

#include "common/log.hpp"
#include "core/events/mouse_event.hpp"
#include "dynamic/layout/flex_layout.hpp"
#include "dynamic/layout/overlayout.hpp"
#include "graphics/cell/painter.hpp"

namespace notf {

//*********************************************************************************************************************/

ScrollArea::ScrollBar::ScrollBar(ScrollArea& scroll_area)
    : size(1)
    , pos(0)
    , m_scroll_area(scroll_area)
{
    Claim claim;
    claim.get_horizontal().set_fixed(10);
    claim.get_horizontal().set_priority(1);
    set_claim(claim);
}

void ScrollArea::ScrollBar::_paint(Painter& painter) const
{
    // background
    const Aabrf widget_rect = Aabrf(get_size());
    painter.begin_path();
    painter.add_rect(widget_rect);
    painter.set_fill(Color("#454950"));
    painter.fill();

    // bar
    if (size < 1) {
        Aabrf bar_rect(widget_rect.get_width(), widget_rect.get_height() * size);
        bar_rect.move_by({0, widget_rect.get_height() * pos});

        painter.begin_path();
        painter.add_rect(bar_rect);
        painter.set_fill(Color("#ffba59"));
        painter.fill();
    }
}

void ScrollArea::Background::_paint(Painter&) const
{
}

//*********************************************************************************************************************/

ScrollArea::ScrollArea()
    : BaseController<ScrollArea>({}, {})
    , m_area_window()
    , m_scroll_container()
    , m_vscrollbar()
    , m_content()
    , m_on_scrollbar_drag()
{
    // the window into the content
    m_area_window = Overlayout::create();
    m_area_window->set_claim(Claim()); // do not combine child Claims
    m_area_window->set_alignment(Overlayout::AlignHorizontal::LEFT, Overlayout::AlignVertical::TOP);

    // transparent background Widget reacting to scroll events not caught by the ScrollArea's content
    std::shared_ptr<Background> background = std::make_shared<Background>();
    m_area_window->add_item(background);

    // container inside the area, scissored by the window and containing the content
    m_scroll_container = Overlayout::create();
    m_scroll_container->set_alignment(Overlayout::AlignHorizontal::LEFT, Overlayout::AlignVertical::TOP);
    m_area_window->add_item(m_scroll_container);
    m_scroll_container->set_scissor(m_area_window.get());

    // the scrollbar on the side of the area window
    m_vscrollbar = std::make_shared<ScrollBar>(*this);

    // the root layout of this Controller
    std::shared_ptr<FlexLayout> root_layout = FlexLayout::create(FlexLayout::Direction::LEFT_TO_RIGHT);
    root_layout->add_item(m_area_window);
    root_layout->add_item(m_vscrollbar);
    _set_root_item(root_layout);
    log_trace << "Scroll container has ID: " << m_scroll_container->get_id();

    // update the scrollbar, if the container layout changed
    connect_signal(
        m_scroll_container->on_aabr_changed,
        [this](const Aabrf&) -> void {
            _update_scrollbar(0);
        });

    // scroll when scrolling the mouse wheel anywhere over the scroll area
    connect_signal(
        root_layout->on_mouse_scroll,
        [this](MouseEvent& event) -> void {
            log_trace << "Derbness";
            _update_scrollbar(16 * event.window_delta.y());
        });

    //    // event handler for when then scrollbar is dragged
    //    m_on_scrollbar_drag = connect_signal(
    //        m_vscrollbar->on_mouse_move,
    //        [this](MouseEvent& event) -> void {
    //            const float area_height = m_area_window->get_grant().height;
    //            if (area_height >= 1) {
    //                _update_scrollbar(event.window_delta.y() * _get_content_height() / area_height);
    //            }
    //            event.set_handled();
    //        });
    //    m_on_scrollbar_drag.disable();

    //    // enable scrollbar dragging when the user clicks on the scrollbar
    //    connect_signal(
    //        m_vscrollbar->on_mouse_button,
    //        [this](MouseEvent& event) -> void {
    //            m_on_scrollbar_drag.enable();
    //            event.set_handled();
    //        },
    //        [this](MouseEvent& event) -> bool {
    //            const float available_height = m_vscrollbar->get_size().height;
    //            const float scroll_bar_top   = m_vscrollbar->get_window_xform().get_translation().y()
    //                + (m_vscrollbar->pos * available_height);
    //            const float scroll_bar_height = m_vscrollbar->size * available_height;
    //            return (event.action == MouseAction::PRESS
    //                    && event.window_pos.y() >= scroll_bar_top
    //                    && event.window_pos.y() <= scroll_bar_top + scroll_bar_height);
    //        });

    //    // disable scrollbar dragging again, when the user releases the mouse
    //    connect_signal(
    //        m_vscrollbar->on_mouse_button,
    //        [this](MouseEvent& event) -> void {
    //            if (m_on_scrollbar_drag.is_enabled()) {
    //                m_on_scrollbar_drag.disable();
    //                event.set_handled();
    //            }
    //        },
    //        [this](MouseEvent& event) -> bool {
    //            return event.action == MouseAction::RELEASE;
    //        });
}

void ScrollArea::set_area_controller(ControllerPtr controller)
{
    m_content = controller;

    m_scroll_container->clear();
    m_scroll_container->add_item(m_content);
}

void ScrollArea::_update_scrollbar(float delta_y)
{
    const float content_height = _get_content_height();
    if (content_height <= precision_high<float>()) {
        return;
    }
    const float area_height = m_area_window->get_grant().height;
    const float overflow    = max(0, content_height - area_height);

    // there must at least be half a pixel to scroll in order for the bar to show up
    if (overflow >= 0.5f) {

        const float container_y     = m_scroll_container->get_xform<ScreenItem::Space::OFFSET>().get_translation().y();
        const float new_container_y = max(area_height - content_height, container_y + delta_y);
        m_scroll_container->set_local_xform(Xform2f::translation(Vector2f(0, new_container_y)));

        m_vscrollbar->size = area_height / content_height;
        m_vscrollbar->pos  = area_height - (abs(new_container_y) / content_height);
    }
    else {
        m_scroll_container->set_local_xform(Xform2f::translation(Vector2f(0, 0/*area_height - content_height*/)));
        m_vscrollbar->size = 1;
        m_vscrollbar->pos  = 0;
    }
}

float ScrollArea::_get_content_height() const
{
    if (!m_content) {
        return 0;
    }
    return m_scroll_container->get_size().height;
}

} // namespace notf
