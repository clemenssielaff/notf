#include "dynamic/controller/scroll_area.hpp"

#include "common/log.hpp"
#include "common/xform2.hpp"
#include "core/events/mouse_event.hpp"
#include "dynamic/layout/overlayout.hpp"
#include "dynamic/layout/stack_layout.hpp"
#include "graphics/cell/painter.hpp"

namespace notf {

ScrollArea::ScrollBar::ScrollBar(ScrollArea& scroll_area)
    : size(.8f)
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
{
}

void ScrollArea::_initialize()
{
    m_area_window = Overlayout::create();
    m_area_window->set_claim(Claim());

    std::shared_ptr<Background> background = std::make_shared<Background>();
    m_area_window->add_item(background);
    background->on_scroll.connect([this](MouseEvent& event) -> void {
        _update_scrollbar(16 * event.window_delta.y);
    });

    m_scroll_container = Overlayout::create();
    m_scroll_container->set_scissor(m_area_window);
    m_scroll_container->size_changed.connect([this](const Size2f&) -> void {
        _update_scrollbar(0);
    });
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

void ScrollArea::_update_scrollbar(float delta_y)
{
    if (!m_content) {
        return;
    }

    Aabrf content_aabr;
    {
        ScreenItemPtr root_item = m_content->get_root_item();
        if (LayoutPtr layout = std::dynamic_pointer_cast<Layout>(root_item)) {
            // TODO: proper bounding-rect method for each layout type
            LayoutIteratorPtr it = layout->iter_items();
            while (const Item* child_item = it->next()) {
                if (const ScreenItem* screen_item = Item::get_screen_item(child_item)) {
                    const Vector2f translation = screen_item->get_transform().get_translation();
                    const Size2f size          = screen_item->get_size();
                    content_aabr.united(Aabrf(translation, size));
                }
            }
        }
        else {
            content_aabr = Aabrf(root_item->get_transform().get_translation(), root_item->get_size());
        }
    }

    const float area_height    = m_scroll_container->get_size().height;
    const float content_height = content_aabr.height();
    const float overflow       = min(0.f, area_height - content_height);

    const float y = min(0.f, max(overflow, m_scroll_container->get_offset_transform().get_translation().y + delta_y));
    m_scroll_container->set_offset_transform(Xform2f::translation({0, y}));

    if (abs(content_height) >= precision_high<float>()) {
        m_vscrollbar->size = area_height / content_height;
        m_vscrollbar->pos  = abs(y) / content_height;
    }
    else {
        m_vscrollbar->size = 1;
        m_vscrollbar->pos  = 0;
    }
}

} // namespace notf
