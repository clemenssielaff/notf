#include "core/layout_item.hpp"

#include "common/log.hpp"
#include "core/render_manager.hpp"
#include "core/window.hpp"

namespace notf {

LayoutItem::~LayoutItem()
{
}

bool LayoutItem::_redraw()
{
    // do not request a redraw, if this item cannot be drawn anyway
    if (!is_visible()) {
        return false;
    }

    if (std::shared_ptr<Window> window = get_window()) {
        window->get_render_manager().request_redraw();
    }
    return true;
}

void LayoutItem::_get_window_transform(Xform2f& result) const
{
    std::shared_ptr<const Item> parent_item = get_parent();
    if (!parent_item) {
        return;
    }
    const LayoutItem* parent_layout;
    if (parent_item->get_layout_item() == this) {
        if (!(parent_item = parent_item->get_parent())) {
            return;
        }
    }

    parent_layout = parent_item->get_layout_item();
    assert(this != parent_layout);
    parent_layout->_get_window_transform(result);
    result = get_transform() * result;
}

} // namespace notf
