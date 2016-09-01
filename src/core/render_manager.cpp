#include "core/render_manager.hpp"

#include "core/components/render_component.hpp"
#include "core/widget.hpp"
#include "core/window.hpp"
#include "graphics/shader.hpp"

namespace signal {

void RenderManager::render(const Window& window)
{
    // lock all widgets for rendering
    std::vector<std::shared_ptr<Widget>> locked_widgets;
    locked_widgets.reserve(m_widgets.size());
    for (const std::weak_ptr<Widget>& weak_widget : m_widgets) {
        std::shared_ptr<Widget> widget = weak_widget.lock();
        if (widget) {
            locked_widgets.emplace_back(widget);
        }
    }

    // render all widgets
    std::set<GLuint> configured_shaders;
    for (const std::shared_ptr<Widget>& widget : locked_widgets) {
        std::shared_ptr<RenderComponent> renderer = std::static_pointer_cast<RenderComponent>(widget->get_component(Component::KIND::RENDER));
        assert(renderer);

        // perform the window setup, if this shader isn't set up yet
        Shader& shader = renderer->get_shader();
        if (!configured_shaders.count(shader.get_id())) {
            shader.use();
            shader.setup_window(window);
            configured_shaders.insert(shader.get_id());
        }

        // render the widget
        renderer->render(*widget.get());
    }
}

} // namespace signal
