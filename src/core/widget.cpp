#include "core/widget.hpp"

#include "common/log.hpp"
#include "common/string.hpp"
#include "core/layout.hpp"
#include "core/window.hpp"
#include "graphics/painter.hpp"
#include "graphics/render_context.hpp"

namespace notf {

AbstractWidget::AbstractWidget()
    : Item()
    , m_scissor_layout() // empty by default
    , m_cell(Cell())
{
    m_cell.set_dirty();
}

void AbstractWidget::set_scissor(std::shared_ptr<Layout> scissor)
{
    if (!has_ancestor(scissor.get())) {
        throw_runtime_error(string_format(
            "Cannot set Layout %i as scissor for Widget %i, because it is not part of the Layout.",
            scissor->id(), id()));
    }
    m_scissor_layout = std::move(scissor);
}

bool AbstractWidget::set_claim(const Claim claim)
{
    bool was_changed = _set_claim(claim);
    if (was_changed) {
        _redraw();
    }
    return was_changed;
}

void AbstractWidget::redraw()
{
    m_cell.set_dirty();
    _redraw();
}

void AbstractWidget::paint(RenderContext& context) const
{
    // update the cell if dirty
    //    if (m_cell.is_dirty()) { TODO: currently _set_size does not dirty the Widget's Cell
    m_cell.reset(context);
    Painter painter(*this, m_cell, context);
    try {
        _paint(painter);
    } catch (std::runtime_error error) {
        log_warning << error.what();
        // TODO: print Python stack trace here IF the item uses a Python object to draw itself
    }
    //    }
    // TODO: clear all m_cell states after painting
}

void AbstractWidget::_widgets_at(const Vector2f& /*local_pos*/, std::vector<AbstractWidget*>& result)
{
    // TODO: Widget::get_widget_at() should test if the given local_pos is loctated in its shape
    result.push_back(this);
}

} // namespace notf
