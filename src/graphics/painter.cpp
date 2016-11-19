#include "graphics/painter.hpp"

#include "common/log.hpp"
#include "core/widget.hpp"

namespace notf {

Painter::Painter(const Widget* widget, const RenderContext* context)
    : m_widget(widget)
    , m_context(context)
    , m_state_count(0)
{
    // save original state
    save_state();

    // create a 'base' state that the user can not pop past
    set_transform(m_widget->get_transform(Space::WINDOW));
    set_scissor(m_widget->get_size());
    save_state();
}

Painter::~Painter()
{
    while (m_state_count > 0) {
        restore_state();
        --m_state_count;
    }
}

Size2f Painter::get_widget_size() const
{
    return m_widget->get_size();
}

Vector2 Painter::get_mouse_pos() const
{
    return m_context->mouse_pos - m_widget->get_transform(Space::WINDOW).get_translation();
}

} // namespace notf
