#include "app/widget/painterpreter.hpp"

#include "app/widget/widget.hpp"
#include "graphics/core/graphics_context.hpp"
#include "graphics/renderer/plotter.hpp"

NOTF_OPEN_NAMESPACE

// ================================================================================================================== //

Painterpreter::Painterpreter(GraphicsContext& context) : m_plotter(std::make_unique<Plotter>(context)) {}

void Painterpreter::paint(const Widget& widget)
{
    m_base_xform = widget.get_xform<Widget::Space::WINDOW>();
    m_base_clipping = widget.get_clipping_rect();
}

void Painterpreter::_push_state() { m_states.emplace_back(m_states.back()); }

void Painterpreter::_pop_state()
{
    assert(m_states.size() > 1);
    m_states.pop_back();
}

NOTF_CLOSE_NAMESPACE
