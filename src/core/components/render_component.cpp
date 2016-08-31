#include "core/components/render_component.hpp"

namespace signal {

RenderComponent::RenderComponent(std::shared_ptr<Shader> shader)
    : Component()
    , m_shader(std::move(shader))
{
}

} // namespace signal
