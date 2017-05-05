#include "dynamic/controller/scroll_area.hpp"

#include "common/log.hpp"
#include "common/xform2.hpp"
#include "core/events/mouse_event.hpp"
#include "dynamic/layout/overlayout.hpp"
#include "dynamic/layout/stack_layout.hpp"
#include "graphics/cell/painter.hpp"

namespace notf {

ScrollArea::ScrollBar::ScrollBar(ScrollArea& scroll_area)
    : size(1)
    , pos(0)
    , m_scroll_area(scroll_area)
{
    Claim::Stretch horizontal, vertical;
    horizontal.set_fixed(10);
    horizontal.set_priority(1);
    set_claim({horizontal, vertical});
}

void ScrollArea::ScrollBar::_paint(Painter& painter) const
{
    // background
    const Aabrf widget_rect = Aabrf(get_size());
    painter.begin_path();
    painter.add_rect(widget_rect);
    painter.set_fill_paint(Color("#454950"));
    painter.fill();

    // bar
    if (size < 1) {
        Aabrf bar_rect(widget_rect.width(), widget_rect.height() * size);
        bar_rect.move_by({0, widget_rect.height() * pos});

        painter.begin_path();
        painter.add_rect(bar_rect);
        painter.set_fill_paint(Color("#ffba59"));
        painter.fill();
    }
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
    , m_on_scrollbar_drag()
{
}

void ScrollArea::_initialize()
{
    // the window into the content
    m_area_window = Overlayout::create();
    m_area_window->set_claim(Claim());

    // transparent background Widget reacting to scroll events not caught by the ScrollArea's content
    std::shared_ptr<Background> background = std::make_shared<Background>();
    m_area_window->add_item(background);

    // container inside the area, scissored by the window and containing the content
    m_scroll_container = Overlayout::create();
    m_scroll_container->set_scissor(m_area_window);
    m_area_window->add_item(m_scroll_container);

    // the scrollbar on the side of the area window
    m_vscrollbar = std::make_shared<ScrollBar>(*this);

    // the root layout of this Controller
    std::shared_ptr<StackLayout> root_layout = StackLayout::create(StackLayout::Direction::LEFT_TO_RIGHT);
    root_layout->add_item(m_area_window);
    root_layout->add_item(m_vscrollbar);
    _set_root_item(root_layout);
    log_trace << "Root Layout has ID: " << root_layout->get_id();

    // update the scrollbar, if the container layout changed
    connect_signal(
        m_scroll_container->on_layout_changed,
        [this]() -> void {
            _update_scrollbar(0);
        });

    // scroll when scrolling the mouse wheel anywhere over the scroll area
    connect_signal(
        root_layout->on_mouse_scroll,
        [this](MouseEvent& event) -> void {
            _update_scrollbar(16 * event.window_delta.y);
        });

    // event handler for when then scrollbar is dragged
    m_on_scrollbar_drag = connect_signal(
        m_vscrollbar->on_mouse_move,
        [this](MouseEvent& event) -> void {
            const float area_height = m_scroll_container->get_size().height;
            if (area_height >= 1) {
                _update_scrollbar(-event.window_delta.y * _get_content_height() / area_height);
            }
            event.set_handled();
        });
    m_on_scrollbar_drag.disable();

    // enable scrollbar dragging when the user clicks on the scrollbar
    connect_signal(
        m_vscrollbar->on_mouse_button,
        [this](MouseEvent& event) -> void {
            m_on_scrollbar_drag.enable();
            event.set_handled();
        },
        [this](MouseEvent& event) -> bool {
            const float scroll_bar_top = m_vscrollbar->get_window_transform().get_translation().y
                + (m_vscrollbar->pos * m_vscrollbar->get_size().height);
            const float scroll_bar_height = m_vscrollbar->size * m_vscrollbar->get_size().height;
            return (event.action == MouseAction::PRESS
                    && event.window_pos.y >= scroll_bar_top
                    && event.window_pos.y <= scroll_bar_top + scroll_bar_height);
        });

    // disable scrollbar dragging again, when the user releases the mouse
    connect_signal(
        m_vscrollbar->on_mouse_button,
        [this](MouseEvent& event) -> void {
            if (m_on_scrollbar_drag.is_enabled()) {
                m_on_scrollbar_drag.disable();
                event.set_handled();
            }
        },
        [this](MouseEvent& event) -> bool {
            return event.action == MouseAction::RELEASE;
        });
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

void ScrollArea::_update_scrollbar(float delta_y)
{
    const float content_height = _get_content_height();
    if (content_height <= precision_high<float>()) {
        return;
    }
    const float area_height = m_scroll_container->get_size().height;
    const float overflow    = min(0.f, area_height - content_height);

    const float y = min(0.f, max(overflow, m_scroll_container->get_offset_transform().get_translation().y + delta_y));
    m_scroll_container->set_offset_transform(Xform2f::translation({0, y}));

    if (overflow <= -0.5f // there must at least be half a pixel to scroll in order for the bar to show up
        && abs(content_height) >= precision_high<float>()) {
        m_vscrollbar->size = area_height / content_height;
        m_vscrollbar->pos  = abs(y) / content_height;
    }
    else {
        m_vscrollbar->size = 1;
        m_vscrollbar->pos  = 0;
    }
}

float ScrollArea::_get_content_height() const
{
    // get content
    if (!m_content) {
        return 0;
    }
    ScreenItemPtr root_item = m_content->get_root_item();
    if (!root_item) {
        log_warning << "Encountered Controller " << m_content->get_id() << " without a root Item";
        return 0;
    }

    // get content aabr
    if (LayoutPtr layout = std::dynamic_pointer_cast<Layout>(root_item)) {
        return layout->get_content_aabr().height();
    }
    else {
        return root_item->get_aarbr().height();
    }
}

} // namespace notf
