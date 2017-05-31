#include "core/widget.hpp"

#include "common/log.hpp"
#include "core/item_container.hpp"
#include "core/layout.hpp"
#include "graphics/cell/cell.hpp"
#include "graphics/cell/cell_canvas.hpp"
#include "graphics/cell/painter.hpp"

namespace notf {

Widget::Widget()
    : ScreenItem(std::make_unique<detail::EmptyItemContainer>())
    , m_cell(std::make_shared<Cell>())
    , m_is_clean(false)
{
}

bool Widget::set_claim(const Claim claim)
{
    bool was_changed = _set_claim(claim);
    if (was_changed) {
        _update_ancestor_layouts();
    }
    return was_changed;
}

void Widget::redraw() const
{
    if (ScreenItem::_redraw()) {
        m_is_clean = false;
    }
}

void Widget::_render(CellCanvas& canvas) const
{
    // update the Cell if the Widget is dirty
//    if (!m_is_clean) {
        Painter painter(canvas, *m_cell.get());
        try {
            _paint(painter);
        } catch (std::runtime_error error) {
            log_warning << error.what();
            return;
        }
        m_is_clean = true;
//    } // TODO: Widget dirty management

    { // paint the Cell
        Scissor scissor;
        if (const Layout* scissor_layout = get_scissor()) {
            scissor.xform = scissor_layout->get_window_transform();

            auto aabr = scissor_layout->get_aabr();
            scissor.xform.transform(aabr);
            scissor.extend = aabr.get_size();
        }
        canvas.paint(*m_cell, get_window_transform(), std::move(scissor));
    }
}

void Widget::_get_widgets_at(const Vector2f& local_pos, std::vector<Widget*>& result) const
{
    if (get_aabr(Space::LOCAL).contains(local_pos)) {
        result.push_back(const_cast<Widget*>(this));
    }
}

} // namespace notf
