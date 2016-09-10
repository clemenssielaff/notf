#pragma once

#include <memory>
#include <set>

#include "common/handle.hpp"

namespace signal {

class Widget;
class Window;

class RenderManager {

public: // methods
    /**
     * \brief Default Constructor.
     */
    explicit RenderManager() = default;

    /**
     * \brief Registers a Widget to be drawn in the next render call.
     * \param widget    Widget to register
     */
    void register_widget(const Handle widget_handle) { m_widgets.emplace(widget_handle); }

    /**
     * \brief Renders all registered Widgets and clears the register.
     */
    void render(const Window& window);

private: // fields
    /**
     * \brief Widgets to draw in the next render call.
     */
    std::set<Handle> m_widgets;
};

} // namespace signal
