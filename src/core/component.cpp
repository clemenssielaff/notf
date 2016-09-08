#include "core/component.hpp"

#include "common/log.hpp"
#include "core/widget.hpp"

namespace signal {

void Component::redraw_widgets()
{
    for (auto& weak_widget : m_widgets) {
        if (auto widget = weak_widget.lock()) {
            widget->redraw();
        } else {
            m_widgets.erase(weak_widget); // std::set allows destructive iteration
        }
    }
}

void Component::register_widget(std::shared_ptr<Widget> widget)
{
    auto result = m_widgets.emplace(widget);
    if (!result.second) {
        log_warning << "Did not register Widget " << widget->get_handle()
                    << " with Component of type " << to_number(get_kind())
                    << " because it is already registered";
    }
}

void Component::unregister_widget(std::shared_ptr<Widget> widget)
{
    auto success = m_widgets.erase(widget);
    if (!success) {
        log_warning << "Did not unregister Widget " << widget->get_handle()
                    << " from Component of type " << to_number(get_kind())
                    << " because it was not registered to begin with";
    }
}

} // namespace signal
