#include "core/widget.hpp"

#include "common/log.hpp"
#include "common/string.hpp"
#include "core/layout.hpp"
#include "core/window.hpp"
#include "graphics/cell/cell.hpp"
#include "graphics/cell/cell_canvas.hpp"
#include "graphics/cell/painter.hpp"
#include "graphics/cell/painterpreter.hpp"
#include "utils/unused.hpp"

namespace notf {

Widget::Widget()
    : ScreenItem()
    , m_scissor_layout() // empty by default
    , m_cell(std::make_shared<Cell>())
    , m_is_clean(false)
{
}

Widget::~Widget()
{
}

void Widget::set_scissor(std::shared_ptr<Layout> scissor)
{
    if (!has_ancestor(scissor.get())) {
        throw_runtime_error(string_format(
            "Cannot set Layout %i as scissor for Widget %i, because it is not part of the Layout.",
            scissor->get_id(), get_id()));
    }
    m_scissor_layout = std::move(scissor);
}

bool Widget::set_claim(const Claim claim)
{
    bool was_changed = _set_claim(claim);
    if (was_changed) {
        _redraw();
    }
    return was_changed;
}

void Widget::redraw() // TODO: currently _set_size does not dirty the Widget's Cell
{
    // don't dirty if the ScreenItem is invisible
    if (!_redraw()) {
        return;
    }
    m_is_clean = false;
}

void Widget::paint(CellCanvas& cell_context) const
{
    // update the Cell if the Widget is dirty
    //    if (!m_is_clean) { // TODO: dirty mechanism for cells
    Painter painter(*m_cell, cell_context, get_window_transform());
    try {
        _paint(painter);
    } catch (std::runtime_error error) {
        log_warning << error.what();
        return;
    }
    m_is_clean = true;
    //    }

    // paint the Cell
    cell_context.paint(*m_cell);
}

void Widget::_get_widgets_at(const Vector2f& local_pos, std::vector<Widget*>& result) const
{
    UNUSED(local_pos);
    result.push_back(const_cast<Widget*>(this));
    // TODO: Widget::get_widget_at() should test if the given local_pos is loctated in its shape
}

} // namespace notf
