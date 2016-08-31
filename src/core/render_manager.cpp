#include "core/render_manager.hpp"

#include "core/components/render_component.hpp"
#include "core/widget.hpp"

namespace signal {

void RenderManager::render()
{
    for (const std::weak_ptr<Widget>& weak_widget : m_widgets) {
        auto widget = weak_widget.lock();
        if (widget) {
            auto renderer = std::dynamic_pointer_cast<RenderComponent>(widget->get_component(Component::KIND::RENDER));
            if (renderer) {
                renderer->render(*widget.get());
            }
        }
    }
    m_widgets.clear();
}

} // namespace signal
