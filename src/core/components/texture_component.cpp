#include "core/components/texture_component.hpp"

namespace signal {

std::shared_ptr<Texture2> TextureComponent::get_texture(ushort channel) const
{
    auto it = m_textures.find(channel);
    if(it == m_textures.end()){
        return {};
    }
    return it->second;
}

} // namespace signal
