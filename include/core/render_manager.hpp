#pragma once

#include <memory>
#include <set>

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
    void register_widget(std::weak_ptr<Widget> widget) { m_widgets.emplace(std::move(widget)); }

    /**
     * \brief Renders all registered Widgets and clears the register.
     */
    void render(const Window& window);

private: // fields
    /**
     * \brief Widgets to draw in the next render call.
     */
    std::set<std::weak_ptr<Widget>, std::owner_less<std::weak_ptr<Widget>> > m_widgets;
};

} // namespace signal
