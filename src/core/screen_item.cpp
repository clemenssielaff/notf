#include "core/screen_item.hpp"

#include "common/log.hpp"
#include "core/layout.hpp"
#include "core/render_manager.hpp"
#include "core/window.hpp"

namespace notf {

ScreenItem::ScreenItem()
    : Item()
    , m_opacity(1)
    , m_size()
    , m_transform(Xform2f::identity())
    , m_claim()
{
}

ScreenItem::~ScreenItem()
{
}

Xform2f ScreenItem::get_window_transform() const
{
    Xform2f result = Xform2f::identity();
    _get_window_transform(result);
    return result;
}

bool ScreenItem::set_opacity(float opacity)
{
    opacity = clamp(opacity, 0, 1);
    if (abs(m_opacity - opacity) <= precision_high<float>()) {
        return false;
    }
    m_opacity = opacity;
    opacity_changed(m_opacity);
    _redraw();
    return true;
}

bool ScreenItem::_redraw()
{
    // do not request a redraw, if this item cannot be drawn anyway
    if (!is_visible()) {
        return false;
    }

    if (std::shared_ptr<Window> my_window = get_window()) {
        my_window->get_render_manager().request_redraw();
    }
    return true;
}

bool ScreenItem::_set_size(const Size2f& size)
{
    if (size != m_size) {
        const Claim::Stretch& horizontal = m_claim.get_horizontal();
        const Claim::Stretch& vertical   = m_claim.get_vertical();
        // TODO: enforce width-to-height constraint in Item::_set_size (the StackLayout does something like it already)

        m_size.width  = max(horizontal.get_min(), min(horizontal.get_max(), size.width));
        m_size.height = max(vertical.get_min(), min(vertical.get_max(), size.height));
        size_changed(m_size);
        _redraw();
        return true;
    }
    return false;
}

bool ScreenItem::_set_transform(const Xform2f transform)
{
    if (transform == m_transform) {
        return false;
    }
    m_transform = std::move(transform);
    transform_changed(m_transform);
    _redraw();
    return true;
}

bool ScreenItem::_set_claim(const Claim claim)
{
    if (claim == m_claim) {
        return false;
    }
    m_claim = std::move(claim);
    return true;
}

void ScreenItem::_get_window_transform(Xform2f& result) const
{
    std::shared_ptr<const Layout> layout = _get_layout();
    if (!layout) {
        return;
    }
    layout->_get_window_transform(result);
    result = get_transform() * result;
}

} // namespace notf
