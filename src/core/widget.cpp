#include "core/widget.hpp"

#include "common/log.hpp"
#include "core/item_container.hpp"
#include "core/layout.hpp"
#include "graphics/cell/cell.hpp"
#include "graphics/cell/cell_canvas.hpp"

namespace notf {

Widget::Widget()
    : ScreenItem(std::make_unique<detail::EmptyItemContainer>())
    , m_cell(std::make_shared<Cell>())
    , m_is_clean(false)
{
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
            scissor.xform = scissor_layout->get_window_xform();

            Aabrf aabr(scissor_layout->get_grant());
            scissor_layout->get_xform<Space::PARENT>().transform(aabr);
            scissor.xform.transform(aabr);
            scissor.extend = aabr.get_size();
        }
        canvas.paint(*m_cell, get_window_xform(), std::move(scissor));
    }
}

void Widget::_relayout()
{
    // a Widget is only concerned about its own size
    Size2f grant = get_grant();
    get_claim().apply(grant);
    _set_size(grant);
}

void Widget::_get_widgets_at(const Vector2f& local_pos, std::vector<Widget*>& result) const
{
    if (Aabrf(get_size()).contains(local_pos)) {
        result.push_back(const_cast<Widget*>(this));
    }
}

} // namespace notf
