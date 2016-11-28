#include "core/render_manager.hpp"

#include "common/log.hpp"
#include "core/application.hpp"
#include "core/components/canvas_component.hpp"
#include "core/object_manager.hpp"
#include "core/state.hpp"
#include "core/widget.hpp"
#include "graphics/rendercontext.hpp"

namespace notf {

void RenderManager::render(const RenderContext& context)
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

    // TODO: perform z-sorting here

    // draw all widgets
    for (const std::shared_ptr<Widget>& widget : widgets) {
        if (widget->get_size().is_zero()) {
            continue;
        }

        std::shared_ptr<CanvasComponent> canvas = widget->get_state()->get_component<CanvasComponent>();
        assert(canvas);
        canvas->render(*widget.get(), context);
    }
}

} // namespace notf
