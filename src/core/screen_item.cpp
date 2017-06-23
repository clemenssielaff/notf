#include "core/screen_item.hpp"

#include "common/log.hpp"
#include "core/item_container.hpp"
#include "core/layout.hpp"
#include "core/render_manager.hpp"
#include "core/window.hpp"

namespace { // anonymous

const float g_alpha_cutoff = 1.f / (255 * 2);

} // namespace anonymous

namespace notf {

ScreenItem::ScreenItem(ItemContainerPtr container)
    : Item(std::move(container))
    , m_layout_transform(Xform4f::identity())
    , m_local_transform(Xform4f::identity())
    , m_effective_transform(Xform4f::identity())
    , m_claim()
    , m_grant(Size2f::zero())
    , m_is_visible(true)
    , m_opacity(1)
    , m_scissor_layout()
    , m_has_explicit_scissor(false)
    , m_render_layer()
    , m_has_explicit_render_layer(false)
{
}

Xform4f ScreenItem::get_window_xform() const
{
    Xform4f result = Xform4f::identity();
    _get_window_transform(result);
    return result;
}

void ScreenItem::set_local_xform(const Xform4f transform)
{
    if (transform == m_local_transform) {
        return;
    }
    m_local_transform = std::move(transform);
    _update_effective_transform();
    _redraw();
}

float ScreenItem::get_opacity(bool effective) const
{
    if (abs(m_opacity) < g_alpha_cutoff) {
        return 0;
    }
    if (effective) {
        if (const Layout* parent_layout = get_layout()) {
            return m_opacity * parent_layout->get_opacity();
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
    // explicitly marked as not visible
    if (!m_is_visible) {
        return false;
    }

    // no window
    if (!get_window()) {
        return false;
    }
    assert(m_scissor_layout);

    // size too small
    if (m_size.width <= precision_high<float>() || m_size.height <= precision_high<float>()) {
        return false;
    }

    // fully transparent
    if (get_opacity() < g_alpha_cutoff) {
        return false;
    }

    { // fully scissored
        Aabrf content_aabr(get_size());
        transformation_between(this, m_scissor_layout).transform(content_aabr);
        Aabrf scissor_aabr(m_scissor_layout->get_grant());
        m_scissor_layout->get_xform<Space::PARENT>().transform(scissor_aabr);
        if (!scissor_aabr.intersects(content_aabr)) {
            return false;
        }
    }

    // visible
    return true;
}

void ScreenItem::set_visible(bool is_visible)
{
    if (is_visible == m_is_visible) {
        return;
    }
    m_is_visible = is_visible;
    on_visibility_changed(m_is_visible);
}

void ScreenItem::set_scissor(const Layout* scissor_layout)
{
    if (!has_ancestor(scissor_layout)) {
        log_critical << "Cannot set Layout " << scissor_layout->get_id() << " as scissor of Item " << get_id()
                     << " because it is not an ancestor of " << get_id();
        scissor_layout = nullptr;
    }
    if (!scissor_layout) {
        if (ScreenItem* parent = get_layout()) {
            scissor_layout = parent->get_scissor();
        }
    }
    _set_scissor(scissor_layout);
    m_has_explicit_scissor = static_cast<bool>(scissor_layout);
}

void ScreenItem::set_render_layer(const RenderLayerPtr& render_layer)
{
    _set_render_layer(render_layer);
    m_has_explicit_render_layer = static_cast<bool>(render_layer);
}

void ScreenItem::_update_from_parent()
{
    Item::_update_from_parent();
    if (Item* parent = get_parent()) {
        Layout* parent_layout = parent->get_layout();
        if (!parent_layout) { // if the parent is the WindowLayout it won't have a parent itself
            parent_layout = dynamic_cast<Layout*>(parent);
        }
        if (parent_layout) { // parent may be a Controller without a parent itself
            _set_scissor(parent_layout->get_scissor());
            _set_render_layer(parent_layout->get_render_layer());
        }
    }
}

bool ScreenItem::_redraw() const
{
    if (!is_visible()) {
        return false;
    }
    Window* window = get_window();
    assert(window);
    window->get_render_manager().request_redraw();
    return true;
}

bool ScreenItem::_set_claim(const Claim claim)
{
    if (claim == m_claim) {
        return false;
    }
    m_claim = std::move(claim);

    // update ancestor layouts
    Layout* layout = get_layout();
    while (layout) {
        // if the parent Layout's Claim changed, we also need to update the grandparent ...
        if (layout->_update_claim()) {
            layout = layout->get_layout();
        }

        // ... otherwise, we have reached the end of the propagation through the ancestry
        // and continue to relayout all children from the parent downwards
        else {
            layout->_relayout();
            break;
        }
    }
    return true;
}

bool ScreenItem::_set_grant(const Size2f grant)
{
    if (grant == m_grant) {
        return false;
    }
    m_grant = std::move(grant);
    _relayout();
    return true;
}

bool ScreenItem::_set_size(const Size2f size)
{
    if(size == m_size){
        return false;
    }
    m_size = std::move(size);
    on_size_changed(m_size);
    _redraw();
    return true;
}

void ScreenItem::_set_layout_xform(const Xform4f transform)
{
    if (transform == m_layout_transform) {
        return;
    }
    m_layout_transform = std::move(transform);
    _update_effective_transform();
    _redraw();
}

void ScreenItem::_set_scissor(const Layout* scissor_layout)
{
    if (scissor_layout == m_scissor_layout) {
        return;
    }
    if (m_has_explicit_scissor) {
        if (has_ancestor(m_scissor_layout)) {
            return;
        }
        m_has_explicit_scissor = false;
    }
    m_scissor_layout = scissor_layout;

    m_children->apply([scissor_layout](Item* item) -> void {
        item->get_screen_item()->_set_scissor(scissor_layout);
    });

    on_scissor_changed(m_scissor_layout);
    _redraw();
}

void ScreenItem::_set_render_layer(const RenderLayerPtr& render_layer)
{
    if (m_has_explicit_render_layer || render_layer == m_render_layer) {
        return;
    }
    m_render_layer = render_layer;

    m_children->apply([render_layer](Item* item) -> void {
        item->get_screen_item()->_set_render_layer(render_layer);
    });

    on_render_layer_changed(m_render_layer);
    _redraw();
}

void ScreenItem::_get_window_transform(Xform4f& result) const
{
    if (const ScreenItem* layout = get_layout()) {
        layout->_get_window_transform(result);
        result.premult(m_effective_transform);
    }
}

void ScreenItem::_update_effective_transform()
{
    m_effective_transform = m_layout_transform * m_local_transform;
    on_xform_changed(m_effective_transform);
}

/**********************************************************************************************************************/

Xform4f transformation_between(const ScreenItem* source, const ScreenItem* target)
{
    const ScreenItem* common_ancestor = source->get_common_ancestor(target)->get_screen_item();
    if (!common_ancestor) {
        std::stringstream ss;
        ss << "Cannot find common ancestor for Items " << source->get_id() << " and " << target->get_id();
        throw std::runtime_error(ss.str());
    }

    Xform4f source_branch = Xform4f::identity();
    for (const ScreenItem* it = source; it != common_ancestor; it = it->get_layout()) {
        source_branch *= it->get_xform<ScreenItem::Space::PARENT>();
    }

    Xform4f target_branch = Xform4f::identity();
    for (const ScreenItem* it = target; it != common_ancestor; it = it->get_layout()) {
        target_branch *= it->get_xform<ScreenItem::Space::PARENT>();
    }
    target_branch.invert();

    source_branch *= target_branch;
    return source_branch;
}

} // namespace notf
