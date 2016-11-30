#include "core/render_manager.hpp"

#include <algorithm>
#include <assert.h>

#include "common/log.hpp"
#include "core/application.hpp"
#include "core/components/canvas_component.hpp"
#include "core/object_manager.hpp"
#include "core/state.hpp"
#include "core/widget.hpp"
#include "graphics/rendercontext.hpp"
#include "utils/smart_enabler.hpp"

namespace notf {

RenderManager::RenderManager()
    : m_dirty_widgets()
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
    // lock all widgets for rendering
    std::vector<std::shared_ptr<Widget>> widgets;
    ObjectManager& item_manager = Application::get_instance().get_object_manager();
    widgets.reserve(m_dirty_widgets.size());
    for (const Handle widget_handle : m_dirty_widgets) {
        if (std::shared_ptr<Widget> widget = item_manager.get_object<Widget>(widget_handle)) {
            widgets.emplace_back(std::move(widget));
        }
    }
    m_dirty_widgets.clear();

    // TODO: perform z-sorting here

    // draw all widgets
    for (const std::shared_ptr<Widget>& widget : widgets) {
        if (widget->get_size().is_zero()) {
            continue;
        }

        std::shared_ptr<CanvasComponent> canvas = widget->get_state()->get_component<CanvasComponent>();
        assert(canvas);
        canvas->render(*widget.get(), context);
    }
}

} // namespace notf
