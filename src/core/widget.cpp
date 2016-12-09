#include "core/widget.hpp"

#include "common/log.hpp"

namespace notf {

std::shared_ptr<Widget> Widget::get_widget_at(const Vector2& /*local_pos*/)
{
    // TODO: Widget::get_widget_at() should test if the given local_pos is loctated in its shape
    return std::static_pointer_cast<Widget>(shared_from_this());
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
