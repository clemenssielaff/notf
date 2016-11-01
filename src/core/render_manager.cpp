#include "core/render_manager.hpp"

#include "core/application.hpp"
#include "core/components/render_component.hpp"
#include "core/object_manager.hpp"
#include "core/widget.hpp"
#include "core/window.hpp"
#include "graphics/shader.hpp"

namespace notf {

void RenderManager::render(const Window& window)
{
    // lock all widgets for rendering
    std::vector<std::shared_ptr<Widget>> widgets;
    ObjectManager& item_manager = Application::get_instance().get_object_manager();
    widgets.reserve(m_widgets.size());
    for (const Handle widget_handle : m_widgets) {
        if (std::shared_ptr<Widget> widget = item_manager.get_object<Widget>(widget_handle)) {
            widgets.emplace_back(std::move(widget));
        }
    }
    m_widgets.clear();

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

} // namespace notf
