#include "core/components/render_component.hpp"

namespace signal {

RenderComponent::RenderComponent(std::shared_ptr<Shader> shader, std::shared_ptr<Texture2> texture)
    : Component()
    , m_shader(std::move(shader))
    , m_texture(std::move(texture))
{
}

} // namespace signal
