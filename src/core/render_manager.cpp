#include "core/render_manager.hpp"

#include "core/application.hpp"
#include "core/components/render_component.hpp"
#include "core/item_manager.hpp"
#include "core/widget.hpp"
#include "core/window.hpp"
#include "graphics/shader.hpp"

namespace signal {

void RenderManager::render(const Window& window)
{
    auto& app = Application::get_instance();

    // lock all widgets for rendering
    std::vector<std::shared_ptr<Widget>> widgets;
    widgets.reserve(m_widgets.size());
    for (const Handle widget_handle : m_widgets) {
        std::shared_ptr<LayoutItem> layout_item = app.get_item_manager().get_item<LayoutItem>(widget_handle);
        if (std::shared_ptr<Widget> widget = std::dynamic_pointer_cast<Widget>(layout_item)) {
            widgets.emplace_back(widget);
        }
    }

    // render all widgets
    std::set<GLuint> configured_renderers;
    for (const std::shared_ptr<Widget>& widget : widgets) {
        auto renderer = widget->get_component<RenderComponent>();
        assert(renderer);

        // perform the window setup, if this shader isn't set up yet
        Shader& shader = renderer->get_shader();
        if (!configured_renderers.count(shader.get_id())) {
            shader.use();
            renderer->setup_window(window);
            configured_renderers.insert(shader.get_id());
        }

        // render the widget
        renderer->render(*widget.get());
    }
}

} // namespace signal
