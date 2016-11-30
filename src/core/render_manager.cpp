#include "core/render_manager.hpp"

#include <algorithm>
#include <assert.h>

#include "common/log.hpp"
#include "core/application.hpp"
#include "core/components/canvas_component.hpp"
#include "core/layout_root.hpp"
#include "core/state.hpp"
#include "core/widget.hpp"
#include "core/window.hpp"
#include "graphics/rendercontext.hpp"
#include "utils/smart_enabler.hpp"

namespace notf {

RenderManager::RenderManager(const Window* window)
    : m_window(window)
    , m_dirty_widgets()
    , m_layers({std::make_shared<MakeSmartEnabler<RenderLayer>>(0)})
    , m_zero_pos(0)
{
}

std::shared_ptr<RenderLayer> RenderManager::create_front_layer()
{
    const int layer_size = static_cast<int>(m_layers.size());
    assert(layer_size > m_zero_pos);
    m_layers.emplace_back(std::make_shared<MakeSmartEnabler<RenderLayer>>(layer_size - m_zero_pos));
    return m_layers.back();
}

std::shared_ptr<RenderLayer> RenderManager::create_back_layer()
{
    ++m_zero_pos;
    m_layers.emplace(m_layers.begin(), std::make_shared<MakeSmartEnabler<RenderLayer>>(m_zero_pos * -1));
    return m_layers.front();
}

std::shared_ptr<RenderLayer> RenderManager::create_layer_above(const std::shared_ptr<RenderLayer>& layer)
{
    const uint other_index = static_cast<uint>(layer->m_order + m_zero_pos);
    if (layer->m_order + m_zero_pos < 0 || other_index >= m_layers.size() || m_layers[other_index] != layer) {
        log_critical << "Cannot insert new layer above unknown RenderLayer";
        return {};
    }

    std::shared_ptr<RenderLayer> result = std::make_shared<MakeSmartEnabler<RenderLayer>>(layer->m_order);
    m_layers.emplace(m_layers.begin() + (other_index + 1), result);
    if (layer->m_order >= 0) {
        for (uint it = other_index + 1; it < m_layers.size(); ++it) {
            m_layers[it]->m_order += 1;
        }
    }
    else {
        ++m_zero_pos;
        for (uint it = 0; it < other_index + 1; ++it) {
            m_layers[it]->m_order -= 1;
        }
    }
    return result;
}

std::shared_ptr<RenderLayer> RenderManager::create_layer_below(const std::shared_ptr<RenderLayer>& layer)
{
    const uint other_index = static_cast<uint>(layer->m_order + m_zero_pos);
    if (layer->m_order + m_zero_pos < 0 || other_index >= m_layers.size() || m_layers[other_index] != layer) {
        log_critical << "Cannot insert new layer below unknown RenderLayer";
        return {};
    }

    std::shared_ptr<RenderLayer> result = std::make_shared<MakeSmartEnabler<RenderLayer>>(layer->m_order);
    m_layers.emplace(m_layers.begin() + other_index, result);
    if (layer->m_order > 0) {
        for (uint it = other_index + 1; it < m_layers.size(); ++it) {
            m_layers[it]->m_order += 1;
        }
    }
    else {
        ++m_zero_pos;
        for (uint it = 0; it < other_index + 1; ++it) {
            m_layers[it]->m_order -= 1;
        }
    }
    return result;
}

void RenderManager::render(const RenderContext& context)
{
    m_dirty_widgets.clear();

    // TODO: clear old RenderLayers here
    for (std::shared_ptr<RenderLayer>& render_layer : m_layers) {
        render_layer->m_widgets.clear();
    }

    LayoutRoot* layout_root = m_window->get_layout_root().get();
    _iterate_layout_hierarchy(layout_root, layout_root->get_render_layer().get());

    // draw all widgets
    for (std::shared_ptr<RenderLayer>& render_layer : m_layers) {
        for (const Widget* widget : render_layer->m_widgets) {
            assert(widget->get_state());
            std::shared_ptr<CanvasComponent> canvas = widget->get_state()->get_component<CanvasComponent>();
            assert(canvas);
            canvas->render(*widget, context);
        }
    }
}

void RenderManager::_iterate_layout_hierarchy(const LayoutItem* layout_item, RenderLayer* parent_layer)
{
    assert(layout_item);
    assert(parent_layer);

    // implicit use of the parent's render layer
    RenderLayer* current_layer = layout_item->get_render_layer().get();
    if (!current_layer) {
        current_layer = parent_layer;
    }

    // if the item is a drawable widget, append it to the current render layer
    if (const Widget* widget = dynamic_cast<const Widget*>(layout_item)) {
        if (widget->get_size().is_zero()) {
            return;
        }
        if (!widget->get_state() || !widget->get_state()->has_component_kind(Component::KIND::CANVAS)) {
            return;
        }
        current_layer->m_widgets.push_back(widget);
    }

    // otherwise start a recursive iteration of the Layout
    else if (const Layout* layout = dynamic_cast<const Layout*>(layout_item)) {
        std::unique_ptr<LayoutIterator> it = layout->iter_items();
        while (const LayoutItem* child_item = it->next()) {
            _iterate_layout_hierarchy(child_item, current_layer);
        }
    }
    else {
        assert(0);
    }
}

} // namespace notf
