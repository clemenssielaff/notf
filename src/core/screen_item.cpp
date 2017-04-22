#include "core/screen_item.hpp"

#include "core/layout.hpp"
#include "core/render_manager.hpp"
#include "core/window.hpp"

namespace notf {

ScreenItem::ScreenItem()
    : Item()
    , m_opacity(1)
    , m_size(Size2f::zero())
    , m_transform(Xform2f::identity())
    , m_claim()
    , m_scissor_layout() // empty by default
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

float ScreenItem::get_opacity(bool own) const
{
    if (own) {
        return m_opacity;
    }
    else {
        return m_opacity * get_layout()->get_opacity();
    }
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

bool ScreenItem::is_visible() const
{
    return m_size.width > precision_high<float>()
        && m_size.height > precision_high<float>()
        && m_opacity > precision_high<float>();
}

LayoutPtr ScreenItem::get_scissor(const bool own) const
{
    if(LayoutPtr scissor = m_scissor_layout.lock()){
        return scissor;
    }
    if (!own) {
        if (LayoutPtr parent = get_layout()) {
            return parent->get_scissor();
        }
    }
    return {};
}

void ScreenItem::set_scissor(LayoutPtr scissor)
{
    m_scissor_layout = std::move(scissor);
}

bool ScreenItem::_redraw()
{
    if (is_visible()) {
        if (std::shared_ptr<Window> my_window = get_window()) {
            my_window->get_render_manager().request_redraw();
            return true;
        }
    }
    return false;
}

bool ScreenItem::_set_size(const Size2f size)
{
    const Claim::Stretch& horizontal = m_claim.get_horizontal();
    const Claim::Stretch& vertical   = m_claim.get_vertical();

    // first, clamp the size to the allowed range
    Size2f actual_size;
    actual_size.width  = min(horizontal.get_max(), max(horizontal.get_min(), size.width));
    actual_size.height = min(vertical.get_max(), max(vertical.get_min(), size.height));

    // then apply ratio constraints
    const std::pair<float, float> ratios = m_claim.get_width_to_height();
    if (ratios.first > precision_high<float>()) {
        // TODO: ScreenItem::_set_size with claim
    }

    if (actual_size == m_size) {
        return false;
    }
    m_size = std::move(actual_size);
    size_changed(m_size);
    _redraw();
    return true;
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
    _set_size(m_size); // update the size to accomodate changed Claim constraints
    return true;
}

void ScreenItem::_get_window_transform(Xform2f& result) const
{
    if (LayoutPtr layout = _get_layout()) {
        layout->_get_window_transform(result);
        result = get_transform() * result;
    }
}

} // namespace notf
