#include "core/screen_item_new.hpp"

#include "common/log.hpp"
#include "core/item_container.hpp"

namespace notf {

Xform2f ScreenItem::get_window_transform() const
{
    Xform2f result = Xform2f::identity();
    _get_window_transform(result);
    return result;
}

void ScreenItem::set_local_transform(const Xform2f transform)
{
    if (transform == m_local_transform) {
        return;
    }
    m_local_transform = std::move(transform);
    _update_effective_transform();
    _redraw();
}

Aabrf ScreenItem::get_aarbr() const
{
    Aabrf aabr(get_size());
    m_effective_transform.transform(aabr);
    return aabr;
}

Aabrf ScreenItem::get_layout_aarbr() const
{
    Aabrf aabr(get_size());
    m_layout_transform.transform(aabr);
    return aabr;
}

Aabrf ScreenItem::get_local_aarbr() const
{
    Aabrf aabr(get_size());
    m_local_transform.transform(aabr);
    return aabr;
}

float ScreenItem::get_opacity(bool effective) const
{
    if (effective) {
        if (const Item* parent = get_parent()) {
            if (abs(m_opacity) <= precision_high<float>()) {
                return 0;
            }
            return m_opacity * parent->get_screen_item()->get_opacity();
        }
    }
    return m_opacity;
}

void ScreenItem::set_opacity(float opacity)
{
    opacity = clamp(opacity, 0, 1);
    if (abs(m_opacity - opacity) <= precision_high<float>()) {
        return;
    }
    m_opacity = opacity;
    on_opacity_changed(m_opacity);
    _redraw();
}

bool ScreenItem::is_visible() const
{
    // size too small
    if (m_size.width <= precision_high<float>() || m_size.height <= precision_high<float>()) {
        return false;
    }

    // RGBA alpha < 0.5
    static const float alpha_cutoff = 1.f / (255 * 2);
    if (m_opacity < alpha_cutoff || get_opacity() < alpha_cutoff) {
        return false;
    }

    // fully scissored
    if (Layout* scissor_layout = m_scissor_layout.lock().get()) {
        Aabrf local_aabr = get_local_aarbr();
        transformation_between(this, scissor_layout).transform(local_aabr);
        if (!scissor_layout->get_local_aarbr().intersects(local_aabr)) {
            return false;
        }
    }

    return true;
}

void ScreenItem::set_render_layer(const RenderLayerPtr& render_layer)
{
    m_has_explicit_render_layer = static_cast<bool>(render_layer);
    _set_render_layer(render_layer);
}

void ScreenItem::_set_parent(Item* parent)
{
    Item::_set_parent(parent);
    if (!m_has_explicit_render_layer) {
        _set_render_layer(parent->get_screen_item()->get_render_layer());
    }
}

void ScreenItem::_set_render_layer(const RenderLayerPtr& render_layer)
{
    if (render_layer == m_render_layer) {
        return;
    }
    m_render_layer = render_layer;

    m_children->apply([render_layer](Item* item) -> void {
        item->get_screen_item()->_set_render_layer(render_layer);
    });

    on_render_layer_changed(m_render_layer);
}

void ScreenItem::_get_window_transform(Xform2f& result) const
{
    if (const ScreenItem* layout = get_layout()) {
        layout->_get_window_transform(result);
        result.premult(m_effective_transform);
    }
}

void ScreenItem::_update_effective_transform()
{
    m_effective_transform = m_layout_transform * m_local_transform;
    on_transform_changed(m_effective_transform);
}

/**********************************************************************************************************************/

Xform2f transformation_between(const ScreenItem* source, const ScreenItem* target)
{

    const ScreenItem* common_ancestor = source->get_common_ancestor(target)->get_screen_item();
    if (!common_ancestor) {
        std::stringstream ss;
        ss << "Cannot find common ancestor for Items " << source->get_id() << " and " << target->get_id();
        throw std::runtime_error(ss.str());
    }

    Xform2f source_branch = Xform2f::identity();
    for (const ScreenItem* it = source; it != common_ancestor; it = it->get_layout()) {
        source_branch *= it->get_transform();
    }

    Xform2f target_branch = Xform2f::identity();
    for (const ScreenItem* it = target; it != common_ancestor; it = it->get_layout()) {
        target_branch *= it->get_transform();
    }
    target_branch.invert();

    source_branch *= target_branch;
    return source_branch;
}

} // namespace notf
