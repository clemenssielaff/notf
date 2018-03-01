#include "app/core/screen_item.hpp"

#include "app/core/layout.hpp"
#include "app/core/window.hpp"
#include "common/log.hpp"

namespace { // anonymous

static constexpr float g_alpha_cutoff = 1.f / (255 * 2);

} // namespace

namespace notf {

ScreenItem::ScreenItem(detail::ItemContainerPtr container)
    : Item(std::move(container))
    , m_layout_transform(Matrix3f::identity())
    , m_offset_transform(Matrix3f::identity())
    , m_claim()
    , m_grant(Size2f::zero())
    , m_size(Size2f::zero())
    , m_content_aabr(Aabrf::zero())
    , m_is_visible(true)
    , m_opacity(1)
    , m_scissor_layout()
    , m_has_explicit_scissor(false)
{}

template<>
const Matrix3f ScreenItem::xform<ScreenItem::Space::WINDOW>() const
{
    Matrix3f result = Matrix3f::identity();
    _window_transform(result);
    return result;
}

void ScreenItem::set_offset_xform(const Matrix3f transform)
{
    if (transform == m_offset_transform) {
        return;
    }
    m_offset_transform = std::move(transform);
    on_xform_changed(m_offset_transform * m_layout_transform);
    _redraw();
}

float ScreenItem::opacity(bool effective) const
{
    if (abs(m_opacity) < g_alpha_cutoff) {
        return 0;
    }
    if (effective) {
        if (const Layout* parent_layout = layout()) {
            return m_opacity * parent_layout->opacity();
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
    if (!window()) {
        return false;
    }
    assert(m_scissor_layout);

    // bounding rect too small
    if (m_size.area() <= precision_low<float>()) {
        return false;
    }

    // fully transparent
    if (opacity() < g_alpha_cutoff) {
        return false;
    }

    { // fully scissored
        Aabrf content_aabr = m_content_aabr;
        transformation_between(this, m_scissor_layout).transform(content_aabr);
        Aabrf scissor_aabr(m_scissor_layout->size());
        m_scissor_layout->xform<Space::PARENT>().transform(scissor_aabr);
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
    _update_parent_layout();
    on_visibility_changed(m_is_visible);
}

void ScreenItem::set_scissor(const Layout* scissor_layout)
{
    if (!has_ancestor(scissor_layout)) {
        log_critical << "Cannot set Layout " << scissor_layout->name() << " as scissor of Item " << name()
                     << " because it is not an ancestor of " << name();
        scissor_layout = nullptr;
    }
    if (!scissor_layout) {
        if (ScreenItem* parent = layout()) {
            scissor_layout = parent->scissor();
        }
    }
    _set_scissor(scissor_layout);
    m_has_explicit_scissor = static_cast<bool>(scissor_layout);
}

void ScreenItem::_update_from_parent()
{
    Item::_update_from_parent();
    if (Item* parent_item = parent()) {
        Layout* parent_layout = parent_item->layout();
        if (!parent_layout) { // if the parent is the WindowLayout it won't have a parent itself
            parent_layout = dynamic_cast<Layout*>(parent_item);
        }
        if (parent_layout) { // parent may be a Controller without a parent itself
            _set_scissor(parent_layout->scissor());
        }
    }
}

bool ScreenItem::_redraw() const
{
    if (!is_visible()) {
        return false;
    }
    Window* my_window = window();
    assert(my_window);
    my_window->request_redraw();
    return true;
}

void ScreenItem::_update_parent_layout()
{
    Layout* parent_layout = layout();
    while (parent_layout) {
        // if the parent Layout's Claim changed, we also need to update the grandparent ...
        if (Layout::Private<ScreenItem>(*parent_layout).update_claim()) {
            parent_layout = parent_layout->layout();
        }

        // ... otherwise, we have reached the end of the propagation through the ancestry
        // and continue to relayout all children from the parent downwards
        else {
            parent_layout->_relayout();
            break;
        }
    }
}

bool ScreenItem::_set_claim(const Claim claim)
{
    if (claim == m_claim) {
        return false;
    }
    m_claim = std::move(claim);
    _update_parent_layout();
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
    if (size == m_size) {
        return false;
    }
    m_size = std::move(size);
    on_size_changed(m_size);
    _redraw();
    return true;
}

void ScreenItem::_set_content_aabr(const Aabrf aabr) { m_content_aabr = std::move(aabr); }

void ScreenItem::_set_layout_xform(const Matrix3f transform)
{
    if (transform == m_layout_transform) {
        return;
    }
    m_layout_transform = std::move(transform);
    on_xform_changed(m_offset_transform * m_layout_transform);
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
        if (ScreenItem* screen_item = item->screen_item()) {
            screen_item->_set_scissor(scissor_layout);
        }
    });

    on_scissor_changed(m_scissor_layout);
    _redraw();
}

void ScreenItem::_window_transform(Matrix3f& result) const
{
    if (const ScreenItem* parent_layout = layout()) {
        parent_layout->_window_transform(result);
        result.premult(m_offset_transform * m_layout_transform);
    }
}

//====================================================================================================================//

Matrix3f transformation_between(const ScreenItem* source, const ScreenItem* target)
{
    const ScreenItem* common_ancestor = source->common_ancestor(target)->screen_item();
    if (!common_ancestor) {
        std::stringstream ss;
        ss << "Cannot find common ancestor for Items " << source->name() << " and " << target->name();
        throw std::runtime_error(ss.str());
    }

    Matrix3f source_branch = Matrix3f::identity();
    for (const ScreenItem* it = source; it != common_ancestor; it = it->layout()) {
        source_branch *= it->xform<ScreenItem::Space::PARENT>();
    }

    Matrix3f tarbranch = Matrix3f::identity();
    for (const ScreenItem* it = target; it != common_ancestor; it = it->layout()) {
        tarbranch *= it->xform<ScreenItem::Space::PARENT>();
    }
    tarbranch.inverse();

    source_branch *= tarbranch;
    return source_branch;
}

} // namespace notf
