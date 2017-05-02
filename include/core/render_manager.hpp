#pragma once

#include <assert.h>
#include <memory>
#include <vector>

#include "common/size2.hpp"

namespace notf {

class RenderLayer;
class RenderManager;
class RenderStats;
class ScreenItem;
class Widget;
class Window;

using RenderLayerPtr = std::shared_ptr<RenderLayer>;

/**********************************************************************************************************************/

/** The RenderLayer class */
class RenderLayer {

    friend class RenderManager;

protected: // factory
    /** Constructor. */
    RenderLayer(const size_t index)
        : m_index(index), m_widgets() {}

    /** Factory */
    static RenderLayerPtr create(const size_t index);

public: // methods
    /** Tests if this RenderLayer is still valid or not. */
    bool is_valid() const { return !m_widgets.empty(); }

    /** Returns the index of this RenderLayer, starting at 0 going up.
     * If there are layers stacked behind the default layer, the default layer will have an index > 0.
     */
    size_t get_index() const { return m_index; }

private: // methods for friends
    /** Invalidates this RenderLayer, if it sticks around longer than its Manager. */
    void invalidate() { m_widgets.clear(); }

private: // fields
    /** Index of this RenderLayer. */
    size_t m_index;

    /** Widgets ordered from back to front. */
    std::vector<const Widget*> m_widgets;
};

/**********************************************************************************************************************/

/** The RenderManager class */
class RenderManager {

public: // methods
    /** Constructor */
    RenderManager(const Window* window);

    /** Destructor. */
    ~RenderManager();

    /** Checks, whether there are any ScreenItems that need to be redrawn. */
    bool is_clean() const { return m_is_clean; }

    /** Returns the number of RenderLayers managed by this manager, is always >=1. */
    size_t get_layer_count() const { return m_layers.size(); }

    /** Returns the default RenderLayer that always exists. */
    RenderLayerPtr get_default_layer() const { return m_default_layer; }

    /** Creates and returns a new RenderLayer at the very front of the stack. */
    RenderLayerPtr create_front_layer();

    /** Creates and returns a new RenderLayer at the very back of the stack. */
    RenderLayerPtr create_back_layer();

    /** Creates and returns a new RenderLayer directly above the given one.
     * @throws std::invalid_argument If the given RenderLayer is not managed by this Manager.
     */
    RenderLayerPtr create_layer_above(const RenderLayerPtr& layer);

    /** Creates and returns a new RenderLayer directly below the given one.
     * @throws std::invalid_argument If the given RenderLayer is not managed by this Manager.
     */
    RenderLayerPtr create_layer_below(const RenderLayerPtr& layer);

    /** Sets the RenderManager dirty so it redraws on the next frame. */
    void request_redraw() { m_is_clean = false; }

    /** Renders all registered Widgets in their correct z-order.
     * Cleans the RenderManager and doesn't render if clean to begin with.
     * @param context   The context into which to render.
     */
    void render(const Size2i buffer_size);

private: // methods
    /** Iterates through all ScreenItems in the Item hierarchy and collects them in their RenderLayers. */
    void _iterate_item_hierarchy(const ScreenItem* screen_item, RenderLayer* parent_layer);

private: // fields
    /** The Window owning this RenderManager. */
    const Window* m_window;

    /** The default layer, will never go out of scope as long as the RenderManager lives. */
    RenderLayerPtr m_default_layer;

    /** All Render layers. */
    std::vector<RenderLayerPtr> m_layers;

    /** Whether the RenderManager needs to update. */
    bool m_is_clean;

    /** Render statistics debug display. */
    std::unique_ptr<RenderStats> m_stats;
};

} // namespace notf
