#include "core/widget.hpp"

#include "common/log.hpp"
#include "core/application.hpp"
#include "core/layout_root.hpp"
#include "core/object_manager.hpp"
#include "core/render_manager.hpp"
#include "core/window.hpp"
#include "utils/smart_enabler.hpp"

namespace notf {

void Claim::Direction::set_min(const Real min)
{
    if (!is_valid(min) || min < 0) {
        log_warning << "Invalid minimum Stretch value: " << min << " - using 0 instead.";
        m_min = 0;
    }
    else {
        m_min = min;
    }
    if (m_min > m_preferred) {
        m_preferred = m_min;
        if (m_min > m_max) {
            m_max = m_min;
        }
    }
}

void Claim::Direction::set_max(const Real max)
{
    if (is_nan(max) || max < 0) {
        log_warning << "Invalid maximum Stretch value: " << max << " - using 0 instead.";
        m_max = 0;
    }
    else {
        m_max = max;
    }
    if (m_max < m_preferred) {
        m_preferred = m_max;
        if (m_max < m_min) {
            m_min = m_max;
        }
    }
}

void Claim::Direction::set_preferred(const Real preferred)
{
    if (!is_valid(preferred) || preferred < 0) {
        log_warning << "Invalid preferred Stretch value: " << preferred << " - using 0 instead.";
        m_preferred = 0;
    }
    else {
        m_preferred = preferred;
    }
    if (m_preferred < m_min) {
        m_min = m_preferred;
    }
    if (m_preferred > m_max) {
        m_max = m_preferred;
    }
}

void Claim::Direction::set_scale_factor(const Real factor)
{
    if (!is_valid(factor) || factor < 0) {
        log_warning << "Invalid Stretch scale factor: " << factor << " - using 0 instead.";
        m_scale_factor = 0;
    }
    else {
        m_scale_factor = factor;
    }
}

void Claim::set_height_for_width(const Real ratio_min, const Real ratio_max)
{
    if (!is_valid(ratio_min) || ratio_min < 0) {
        log_warning << "Invalid min ratio: " << ratio_min << " - using 0 instead.";
        if (!is_nan(ratio_max)) {
            log_warning << "Ignoring ratio_max value, since the min ratio constraint is set to 0.";
        }
        m_height_for_width = {0, 0};
        return;
    }

    if (is_nan(ratio_max)) {
        m_height_for_width = {ratio_min, ratio_min};
        return;
    }

    if (ratio_max < ratio_min) {
        log_warning << "Ignoring ratio_max value " << ratio_max
                    << ", since it is smaller than the min_ratio " << ratio_min;
        m_height_for_width = {ratio_min, ratio_min};
        return;
    }

    if (approx(ratio_min, 0)) {
        log_warning << "Ignoring ratio_max value, since the min ratio constraint is set to 0.";
        m_height_for_width = {ratio_min, ratio_min};
        return;
    }

    m_height_for_width = {ratio_min, ratio_max};
}

std::shared_ptr<Window> Widget::get_window() const
{
    std::shared_ptr<const LayoutRoot> root_item = get_root();
    if (!root_item) {
        log_critical << "Cannot determine Window for unrooted Widget " << get_handle();
        return {};
    }
    return root_item->get_window();
}

void Widget::add_component(std::shared_ptr<Component> component)
{
    if (!component) {
        log_critical << "Cannot add invalid Component to Widget " << get_handle();
        return;
    }
    remove_component(component->get_kind());
    component->register_widget(get_handle());
    m_components[component->get_kind()] = std::move(component);
}

void Widget::remove_component(Component::KIND kind)
{
    if (!has_component_kind(kind)) {
        return;
    }
    auto it = m_components.find(kind);
    assert(it != m_components.end());
    it->second->unregister_widget(get_handle());
    m_components.erase(it);
}

void Widget::redraw()
{
    // don't draw invisible widgets
    if (get_visibility() != VISIBILITY::VISIBLE) {
        return;
    }

    // if this Widget is renderable, register yourself to be rendered in the next frame
    if (has_component_kind(Component::KIND::RENDER)) {
        std::shared_ptr<Window> window = get_window();
        assert(window);
        window->get_render_manager().register_widget(get_handle());
    }
}

std::shared_ptr<Widget> Widget::get_widget_at(const Vector2& /*local_pos*/)
{
    // if this Widget has no shape, you cannot find it at any location
    if(!has_component_kind(Component::KIND::SHAPE)){
        return {};
    }

    // TODO: Widget::get_widget_at() should test if the given local_pos is loctated in its shape
    return std::static_pointer_cast<Widget>(shared_from_this());
}

} // namespace notf
