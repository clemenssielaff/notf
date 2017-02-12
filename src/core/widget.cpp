#include "core/widget.hpp"

#include "common/log.hpp"
#include "core/window.hpp"
#include "graphics/painter.hpp"
#include "graphics/render_context.hpp"

namespace notf {

bool Widget::get_widgets_at(const Vector2 /*local_pos*/, std::vector<Widget*>& result)
{
    // TODO: Widget::get_widget_at() should test if the given local_pos is loctated in its shape
    result.push_back(this);
    return true;
}

bool Widget::set_claim(const Claim claim)
{
    bool was_changed = _set_claim(claim);
    if (was_changed) {
        _redraw();
    }
    return was_changed;
}

Widget::Widget()
    : LayoutItem()
    , m_scissor_layout() // empty by default
    , m_cell(Cell())
{
    m_cell.set_dirty();
}

void Widget::paint(RenderContext &context) const
{
    // update the cell if dirty
//    if (m_cell.is_dirty()) { TODO: currently _set_size does not dirty the Widget's Cell
        m_cell.reset(context);
        Painter painter(*this, m_cell, context);
        try {
            _paint(painter);
        }
        catch (std::runtime_error error) {
            log_warning << error.what();
            // TODO: print Python stack trace here IF the item uses a Python object to draw itself
        }
//    }
        // TODO: clear all m_cell states after painting
}

} // namespace notf
