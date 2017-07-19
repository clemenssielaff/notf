#include "test_utils.hpp"

namespace notf {

RectWidget::RectWidget(const float width, const float height)
    : Widget()
{
    Claim::Stretch horizontal, vertical;
    horizontal.set_fixed(width);
    vertical.set_fixed(height);
    set_claim({horizontal, vertical});
}

void RectWidget::_paint(Painter&) const {}

} // namespace notf
