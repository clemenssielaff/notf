#include "core/render_manager.hpp"

#include <algorithm>

#include "common/log.hpp"
#include "core/layout_root.hpp"
#include "core/widget.hpp"
#include "core/window.hpp"
#include "graphics/painter.hpp"
#include "graphics/rendercontext.hpp"
#include "utils/make_smart_enabler.hpp"

namespace notf {

RenderManager::RenderManager(const Window* window)
    : m_window(window)
    , m_default_layer(std::make_shared<MakeSmartEnabler<RenderLayer>>())
    , m_layers({m_default_layer})
    , m_is_clean(false)
{
}

std::shared_ptr<RenderLayer> RenderManager::create_front_layer()
{
    m_layers.emplace_back(std::make_shared<MakeSmartEnabler<RenderLayer>>());
    return m_layers.back();
}

std::shared_ptr<RenderLayer> RenderManager::create_back_layer()
{
    m_layers.emplace(m_layers.begin(), std::make_shared<MakeSmartEnabler<RenderLayer>>());
    return m_layers.front();
}

std::shared_ptr<RenderLayer> RenderManager::create_layer_above(const std::shared_ptr<RenderLayer>& layer)
{
    auto it = std::find(m_layers.begin(), m_layers.end(), layer);
    if (it == m_layers.end()) {
        log_critical << "Cannot insert new layer above unknown RenderLayer";
        return {};
    }

    std::shared_ptr<RenderLayer> result = std::make_shared<MakeSmartEnabler<RenderLayer>>();
    m_layers.emplace(++it, result);
    return result;
}

std::shared_ptr<RenderLayer> RenderManager::create_layer_below(const std::shared_ptr<RenderLayer>& layer)
{
    auto it = std::find(m_layers.begin(), m_layers.end(), layer);
    if (it == m_layers.end()) {
        log_critical << "Cannot insert new layer below unknown RenderLayer";
        return {};
    }

    std::shared_ptr<RenderLayer> result = std::make_shared<MakeSmartEnabler<RenderLayer>>();
    m_layers.emplace(it, result);
    return result;
}

void RenderManager::render(const RenderContext& context)
{
    // remove unused layers
    std::remove_if(m_layers.begin(), m_layers.end(), [](std::shared_ptr<RenderLayer>& layer) -> bool {
        return layer.unique();
    });

    // register all drawable widgets with their render layers
    LayoutRoot* layout_root = m_window->get_layout_root().get();
    _iterate_layout_hierarchy(layout_root, layout_root->get_render_layer().get());

    // draw all widgets
    for (std::shared_ptr<RenderLayer>& render_layer : m_layers) {
        for (const Widget* widget : render_layer->m_widgets) {
            Painter painter(widget, &context);
            try {
                widget->paint(painter);
            }
            catch (std::runtime_error error) {
                log_warning << error.what();
            }
        }
        render_layer->m_widgets.clear();
    }

    m_is_clean = true;
}

void RenderManager::_iterate_layout_hierarchy(const Item* item, RenderLayer* parent_layer)
{
    assert(item);
    assert(parent_layer);

    // ignore invisible items
    if (!item->is_visible()) {
        return;
    }

    // implicit use of the parent's render layer
    RenderLayer* current_layer = item->get_render_layer().get();
    if (!current_layer) {
        current_layer = parent_layer;
    }

    // if the item is a drawable widget, append it to the current render layer
    if (const Widget* widget = dynamic_cast<const Widget*>(item)) {
        if (widget->get_size().is_zero()) {
            return;
        }
        current_layer->m_widgets.push_back(widget);
    }

    // otherwise start a recursive iteration of the Layout
    else if (const Layout* layout = dynamic_cast<const Layout*>(item)) {
        std::unique_ptr<LayoutIterator> it = layout->iter_items();
        while (const Item* child_item = it->next()) {
            _iterate_layout_hierarchy(child_item, current_layer);
        }
    }
    else {
        assert(0);
    }
}

} // namespace notf
