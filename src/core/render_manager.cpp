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

    { // remove unused layers
        const size_t size_before = m_layers.size();
        std::remove_if(m_layers.begin(), m_layers.end(), [](RenderLayerPtr& layer) -> bool {
            return layer.unique();
        });
        if (size_before != m_layers.size()) {
            size_t index = 0;
            for (RenderLayerPtr& layer : m_layers) {
                layer->m_index = index++;
            }
        }
    }

    { // draw all widgets
        std::vector<std::vector<const Widget*>> widgets(m_layers.size());
        _collect_widgets(m_window->get_layout().get(), widgets);
        for (const auto& render_layer : widgets) {
            for (const Widget* widget : render_layer) {
                widget->paint(cell_context);
            }
        }
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

void RenderManager::_collect_widgets(const ScreenItem* root_item, std::vector<std::vector<const Widget*>>& widgets)
{
    assert(root_item);

    // don't draw invisible Widgets
    if (!root_item->is_visible()) {
        return;
    }

    // don't draw scissored Widgets
//    if(LayoutPtr scissor = root_item->get_scissor()){
//        Aabrf root_item_aabr = root_item->get_local_aarbr();
//        get_transformation_between(root_item, scissor.get()).transform(root_item_aabr);
//        if(!scissor->get_local_aarbr().intersects(root_item_aabr)){
//            return;
//        }
//    }
    // TODO: ignoring scissored widgets doesn't work...

    size_t render_layer = root_item->get_render_layer()->get_index();
    assert(render_layer < widgets.size());

    // if the item is a visible widget, append it to the current render layer
    if (const Widget* widget = dynamic_cast<const Widget*>(root_item)) {
        widgets[render_layer].push_back(widget);
    }

    // if it is a Layout, start a recursive iteration
    else if (const Layout* layout = dynamic_cast<const Layout*>(root_item)) {
        LayoutIteratorPtr it = layout->iter_items();
        while (const Item* child_item = it->next()) {
            if (const ScreenItem* screen_item = get_screen_item(child_item)) {
                _collect_widgets(screen_item, widgets);
            }
        }
    }

    else { // a ScreenItem but not a Layout or a Widget? .. something's wrong...
        assert(false);
    }
}

} // namespace notf
