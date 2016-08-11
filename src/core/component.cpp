#include "core/component.hpp"

#include "core/application.hpp"

namespace signal {

Component::Component()
    : m_is_dirty{ false }
{
}

Component::~Component()
{
}

void Component::set_dirty()
{
    // no need to register twice
    if (m_is_dirty) {
        return;
    }
    m_is_dirty = true;
    Application::get_instance().register_dirty_component(std::move(shared_from_this()));
}

} // namespace signal
