#include "core/render_manager.hpp"

#include "common/log.hpp"
#include "common/time.hpp"
#include "core/layout_root.hpp"
#include "core/widget.hpp"
#include "core/window.hpp"
#include "graphics/stats.hpp"
#include "utils/make_smart_enabler.hpp"

namespace notf {

RenderManager::RenderManager(const Window* window)
    : m_window(window)
    , m_default_layer(std::make_shared<MakeSmartEnabler<RenderLayer>>())
    , m_layers({m_default_layer})
    , m_is_clean(false)
    , m_stats()
{
    //m_stats = std::make_unique<RenderStats>(120);
}

RenderManager::~RenderManager() = default; // without this, the forward declared unique_ptr to RenderStats wouldn't work

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

void RenderManager::render(RenderContext& context)
{
    Time time_at_start = Time::now();

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
            widget->paint(context);
        }
        render_layer->m_widgets.clear();
    }
    m_is_clean = true;

    // draw the render stats on top
    if (m_stats) {
        double time_elapsed = (Time::now().since(time_at_start)).in_seconds();
        m_stats->update(static_cast<float>(time_elapsed));
        m_stats->render_stats(context);
    }
}

void RenderManager::_iterate_layout_hierarchy(const LayoutItem* item, RenderLayer* parent_layer)
{
    assert(item);
    assert(parent_layer);

    // implicit use of the parent's render layer
    RenderLayer* current_layer = item->get_render_layer().get();
    if (!current_layer) {
        current_layer = parent_layer;
    }

    // if the item is a visible widget, append it to the current render layer
    if (const Widget* widget = dynamic_cast<const Widget*>(item)) {
        // TODO: dont' draw scissored widgets
        if (!widget->is_visible()) {
            return;
        }
        current_layer->m_widgets.push_back(widget);
    }

    // if it is a Layout, start a recursive iteration
    else if (const Layout* layout = dynamic_cast<const Layout*>(item)) {
        if (!layout->is_visible()) {
            return;
        }
        std::unique_ptr<LayoutIterator> it = layout->iter_items();
        while (const Item* child_item = it->next()) {
            if (const LayoutItem* layout_item = child_item->get_layout_item()) {
                _iterate_layout_hierarchy(layout_item, current_layer);
            }
        }
    }

    else {
        assert(false);
    }
}

} // namespace notf
