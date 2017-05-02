#include "core/render_manager.hpp"

#include <iterator>

#include "common/log.hpp"
#include "common/time.hpp"
#include "common/vector.hpp"
#include "core/controller.hpp"
#include "core/widget.hpp"
#include "core/window.hpp"
#include "core/window_layout.hpp"
#include "graphics/cell/cell_canvas.hpp"
#include "graphics/stats.hpp"

namespace notf {

/**********************************************************************************************************************/

RenderLayerPtr RenderLayer::create(const size_t index)
{
    struct make_shared_enabler : public RenderLayer {
        make_shared_enabler(const size_t index)
            : RenderLayer(std::forward<const size_t>(index)) {}
    };
    return std::make_shared<make_shared_enabler>(index);
}

/**********************************************************************************************************************/

RenderManager::RenderManager(const Window* window)
    : m_window(window)
    , m_default_layer(RenderLayer::create(0))
    , m_layers({m_default_layer})
    , m_is_clean(false)
    , m_stats()
{
    m_stats = std::make_unique<RenderStats>(120);
}

RenderManager::~RenderManager()
{
    for (RenderLayerPtr& layer : m_layers) {
        layer->invalidate();
    }
}

RenderLayerPtr RenderManager::create_front_layer()
{
    RenderLayerPtr result = RenderLayer::create(m_layers.size());
    m_layers.emplace_back(result);
    return result;
}

RenderLayerPtr RenderManager::create_back_layer()
{
    return create_layer_below(m_layers.front());
}

RenderLayerPtr RenderManager::create_layer_above(const RenderLayerPtr& layer)
{
    auto it                = std::find(m_layers.begin(), m_layers.end(), layer);
    const size_t new_index = static_cast<size_t>(std::distance(m_layers.begin(), it)) + 1;
    if (new_index >= m_layers.size()) {
        throw std::invalid_argument("Cannot insert new layer above unknown RenderLayer");
    }

    RenderLayerPtr result = RenderLayer::create(new_index);
    m_layers.emplace(++it, result);
    for (; it != m_layers.end(); ++it) {
        ++((*it)->m_index);
    }
    return result;
}

RenderLayerPtr RenderManager::create_layer_below(const RenderLayerPtr& layer)
{
    auto it                = std::find(m_layers.begin(), m_layers.end(), layer);
    const size_t new_index = static_cast<size_t>(std::distance(m_layers.begin(), it));
    if (new_index >= m_layers.size()) {
        throw std::invalid_argument("Cannot insert new layer below unknown RenderLayer");
    }

    RenderLayerPtr result = RenderLayer::create(new_index);
    m_layers.emplace(it, result);
    for (++it; it != m_layers.end(); ++it) {
        ++((*it)->m_index);
    }
    return result;
}

void RenderManager::render(const Size2i buffer_size)
{
    // TODO: optimize case where there's just one layer and you can simply draw them as you iterate through them

    Time time_at_start = Time::now();

    // prepare the render context
    CellCanvas& cell_context = m_window->get_cell_context();
    cell_context.begin_frame(buffer_size, time_at_start, m_window->get_mouse_pos());

    // remove unused layers
    std::remove_if(m_layers.begin(), m_layers.end(), [](RenderLayerPtr& layer) -> bool {
        return layer.unique();
    });

    // register all drawable widgets with their render layers
    WindowLayout* window_layout = m_window->get_layout().get();
    _iterate_item_hierarchy(window_layout, get_default_layer().get());

    // draw all widgets
    for (RenderLayerPtr& render_layer : m_layers) {
        for (const Widget* widget : render_layer->m_widgets) {
            widget->paint(cell_context);
        }
        render_layer->m_widgets.clear();
    }
    m_is_clean = true;

    // draw the render stats on top
    if (m_stats) {
        double time_elapsed = (Time::now().since(time_at_start)).in_seconds();
        m_stats->update(static_cast<float>(time_elapsed));
        m_stats->render_stats(cell_context);
    }

    // flush
    cell_context.finish_frame();
}

void RenderManager::_iterate_item_hierarchy(const ScreenItem* screen_item, RenderLayer* parent_layer)
{
    assert(screen_item);
    assert(parent_layer);

    // TODO: dont' draw scissored widgets
    if (!screen_item->is_visible()) {
        return;
    }

    // implicit use of the parent's render layer
    RenderLayer* current_layer = screen_item->get_render_layer(true).get();
    if (!current_layer) {
        current_layer = parent_layer;
    }

    // if the item is a visible widget, append it to the current render layer
    if (const Widget* widget = dynamic_cast<const Widget*>(screen_item)) {
        current_layer->m_widgets.push_back(widget);
    }

    // if it is a Layout, start a recursive iteration
    else if (const Layout* layout = dynamic_cast<const Layout*>(screen_item)) {
        LayoutIteratorPtr it = layout->iter_items();
        while (const Item* child_item = it->next()) {
            if (const ScreenItem* screen_item = Item::get_screen_item(child_item)) {
                _iterate_item_hierarchy(screen_item, current_layer);
            }
        }
    }

    else { // a ScreenItem but not a Layout or a Widget? .. something's wrong...
        assert(false);
    }
}

} // namespace notf
