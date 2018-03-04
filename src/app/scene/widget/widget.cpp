#include "app/scene/widget/widget.hpp"

#include "common/log.hpp"

NOTF_OPEN_NAMESPACE

Widget::Widget() : ScreenItem(std::make_unique<detail::EmptyItemContainer>()) {}

void Widget::redraw() const { ScreenItem::_redraw(); }

void Widget::_relayout()
{
    // a Widget is only concerned about its own size
    Size2f grant = claim().apply(size());
    _set_size(grant);
    _set_content_aabr(grant);
}

// void Widget::_render(CellCanvas& canvas) const
//{
//    // update the Cell if the Widget is dirty
//    //    if (!m_is_clean) {
//    Painter painter(canvas, *m_cell.get());
//    try {
//        _paint(painter);
//    }
//    catch (std::runtime_error error) {
//        log_warning << error.what();
//        return;
//    }
//    m_is_clean = true;
//    //    } // TODO: Widget dirty management

//    { // paint the Cell
//        Scissor scissor;
//        if (const Layout* scissor_layout = scissor()) {
//            scissor.xform = scissor_layout->xform<Space::WINDOW>();

//            Aabrf aabr(scissor_layout->grant());
//            scissor_layout->xform<Space::PARENT>().transform(aabr);
//            scissor.xform.transform(aabr);
//            scissor.extend = aabr.size();
//        }
//        canvas.paint(*m_cell, xform<Space::WINDOW>(), std::move(scissor), opacity(true));
//    }
//}

void Widget::_widgets_at(const Vector2f& local_pos, std::vector<Widget*>& result) const
{
    if (Aabrf(size()).contains(local_pos)) {
        result.push_back(const_cast<Widget*>(this));
    }
}

NOTF_CLOSE_NAMESPACE
