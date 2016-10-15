#include "core/component.hpp"

#include "common/log.hpp"
#include "core/application.hpp"
#include "core/item_manager.hpp"
#include "core/widget.hpp"
#include "utils/enum_to_number.hpp"

namespace signal {

void Component::redraw_widgets()
{
    for (Handle widget_handle : m_widgets) {
        if (auto widget = Application::get_instance().get_item_manager().get_item<Widget>(widget_handle)) {
            widget->redraw();
        }
        else {
            m_widgets.erase(widget_handle); // std::set allows destructive iteration
        }
    }
}

void Component::register_widget(Handle widget_handle)
{
    auto result = m_widgets.emplace(widget_handle);
    if (!result.second) {
        log_warning << "Did not register Widget " << widget_handle
                    << " with Component of type " << to_number(get_kind())
                    << " because it is already registered";
    }
}

void Component::unregister_widget(Handle widget_handle)
{
    auto success = m_widgets.erase(widget_handle);
    if (!success) {
        log_warning << "Did not unregister Widget " << widget_handle
                    << " from Component of type " << to_number(get_kind())
                    << " because it was not registered to begin with";
    }
}

} // namespace signal
