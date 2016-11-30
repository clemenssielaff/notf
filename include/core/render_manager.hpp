#pragma once

#include <memory>
#include <set>
#include <vector>

#include "common/handle.hpp"

namespace notf {

class LayoutItem;
struct RenderContext;
class Widget;
class Window;

/**********************************************************************************************************************/

/**
 * @brief The RenderLayer class
 */
class RenderLayer {

    friend class RenderManager;

protected: // methods
    explicit RenderLayer(const int order)
        : m_order(order)
    {
    }

private: // fields
    /** Order of this RenderLayer relative to the default at 0. */
    int m_order;

    /** Widgets ordered from back to front. */
    std::vector<const Widget*> m_widgets;
};

/**********************************************************************************************************************/

/**
 * @brief The RenderManager class
 */
class RenderManager {

protected: // methods
    explicit RenderManager(const Window* window);

public: // methods
    /** Checks, whether there are any LayoutItems that need to be redrawn. */
    bool is_clean() const { return m_dirty_widgets.empty(); }

    /** Returns the default RenderLayer that always exists. */
    std::shared_ptr<RenderLayer> get_default_layer() const { return m_layers[static_cast<unsigned>(m_zero_pos)]; }

    /** Creates and returns a new RenderLayer at the very front of the stack. */
    std::shared_ptr<RenderLayer> create_front_layer();

    /** Creates and returns a new RenderLayer at the very back of the stack. */
    std::shared_ptr<RenderLayer> create_back_layer();

    /** Creates and returns a new RenderLayer directly above the given one. */
    std::shared_ptr<RenderLayer> create_layer_above(const std::shared_ptr<RenderLayer>& layer);

    /** Creates and returns a new RenderLayer directly below the given one. */
    std::shared_ptr<RenderLayer> create_layer_below(const std::shared_ptr<RenderLayer>& layer);

    /** Registers a Widgets to be drawn in the next render call.
     * @param widget_handle     Handle of the Widget to register.
     */
    void register_dirty(const Handle widget_handle) { m_dirty_widgets.emplace(widget_handle); }

    /** Unregisters a Widget from being drawn in the next render call.
     * Widgets that weren't registered to begin with, stay that way.
     * @param widget_handle     Handle of the Widget to unregister.
     */
    void register_clean(const Handle widget_handle) { m_dirty_widgets.erase(widget_handle); }

    /** Renders all registered Widgets in their correct z-order.
     * Afterwards, the RenderManager is clean again.
     * @param context   The context into which to render.
     */
    void render(const RenderContext& context);

private: // methods

    void _iterate_layout_hierarchy(const LayoutItem *layout_item, RenderLayer* parent_layer);

private: // fields
    /** The Window owning this RenderManager. */
    const Window* m_window;

    /** Widgets that registered themselves as being dirty. */
    std::set<Handle> m_dirty_widgets;

    /** All Render layers. */
    std::vector<std::shared_ptr<RenderLayer>> m_layers;

    /** Position of the default layer in `m_layers`. */
    int m_zero_pos;
};

// TODO: I don't think I need the RenderLayer order at all... that would greatly simplify inserting new Layers
// TODO: also there is currently no way of removing old layers (even though they are really lightweigt we might want to do that every so often

} // namespace notf
