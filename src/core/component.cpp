#include "core/component.hpp"

#include "common/log.hpp"
#include "core/widget.hpp"
#include "utils/enum_to_number.hpp"

namespace notf {

void Component::_redraw_widgets()
{
    for (auto it = m_widgets.begin(); it != m_widgets.end();) {
        if (auto widget = it->lock()) {
            widget->redraw();
            ++it;
        }
        else {
            it = m_widgets.erase(it); // std::set allows destructive iteration
        }
    }
}

void Component::_register_widget(std::shared_ptr<Widget> widget)
{
    auto result = m_widgets.emplace(widget);
    if (!result.second) {
        log_warning << "Did not register Widget " << widget
                    << " with Component of type " << to_number(get_kind())
                    << " because it is already registered";
    }
}

void Component::_unregister_widget(std::shared_ptr<Widget> widget)
{
    auto success = m_widgets.erase(widget);
    if (!success) {
        log_warning << "Did not unregister Widget " << widget
                    << " from Component of type " << to_number(get_kind())
                    << " because it was not registered to begin with";
    }
}

} // namespace notf
