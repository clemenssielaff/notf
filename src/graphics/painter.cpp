#include <assert.h>

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
    assert(nvgNumStates(_get_context()) == 1);
    save_state();

    // create a 'base' state that the user can not pop past
    auto trans = m_widget->get_transform(Space::WINDOW);
    auto scissor = m_widget->get_size();
    set_transform(trans);
//    set_scissor(scissor);
    save_state();
}

Painter::~Painter()
{
    while (m_state_count > 0) {
        nvgRestore(_get_context());
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
