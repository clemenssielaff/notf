#include "core/widget.hpp"

#include "common/log.hpp"

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
{
}

} // namespace notf
