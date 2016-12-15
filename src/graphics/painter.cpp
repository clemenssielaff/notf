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
    set_transform(m_widget->get_window_transform());
    set_scissor(m_widget->get_size()); // TODO: implement scissor hierarchy
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
    return m_context->mouse_pos - m_widget->get_window_transform().get_translation();
}

void Painter::restore_state()
{
    if (m_state_count > 2) { // do not pop original or base-state (see constructor for details)
        nvgRestore(_get_context());
    }
    else {
        log_warning << "No states left to restore";
    }
}

} // namespace notf
