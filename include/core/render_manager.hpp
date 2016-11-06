#pragma once

#include <set>

#include "common/handle.hpp"

namespace notf {

struct RenderContext;
class Window;

/**********************************************************************************************************************/

class RenderManager {

public: // methods
    explicit RenderManager() = default;

    /** Checks, whether there are any LayoutItems that need to be redrawn. */
    bool is_clean() const { return m_widgets.empty(); }

    /** Registers a Widgets to be drawn in the next render call.
     * @param widget_handle     Handle of the Widget to register.
     */
    void register_widget(const Handle widget_handle) { m_widgets.emplace(widget_handle); }

    /** Unregisters a Widget from being drawn in the next render call.
     * Widgets that weren't registered to begin with, stay that way.
     * @param widget_handle     Handle of the Widget to unregister.
     */
    void unregister_widget(const Handle widget_handle) { m_widgets.erase(widget_handle); }

    /** Renders all registered Widgets in their correct z-order.
     * Afterwards, the RenderManager is clean again.
     * @param context   The context into which to render.
     */
    void render(const RenderContext& context);

private: // methods
    /** Removes all dirty items. */
    void set_clean() { m_widgets.clear(); }

private: // fields
    /** Widgets that registered themselves as being dirty. */
    std::set<Handle> m_widgets;
};

} // namespace notf
