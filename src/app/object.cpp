#include "app/object.hpp"

#include <assert.h>

namespace untitled {

bool Object::attach_component(shared_ptr<Component> component)
{
    const Component::KIND component_kind = component->get_kind();
    if (has_component_kind(component_kind)) {
        assert(false);
        return false;
    }

    m_components.push_back(component);
    m_component_kinds.set(static_cast<size_t>(component_kind));
    return true;
}

} // namespace untitled
