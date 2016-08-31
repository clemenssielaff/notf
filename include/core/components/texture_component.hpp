#pragma once

#include <memory>
#include <unordered_map>

#include "core/component.hpp"

namespace signal {

class Texture2;

/// \brief A Texture Component contains 0-n Textures that are identifiable by their Texture Channel.
///
class TextureComponent : public Component {

public: // methods
    /// \brief This Component's type.
    virtual KIND get_kind() const override { return KIND::TEXTURE; }

    /// \brief Returns the Texture of the requested channel.
    /// \param channel  Texture channel
    /// \return The requested Texture, is empty if the Component doesn't provide the given channel.
    std::shared_ptr<Texture2> get_texture(ushort channel) const;

    /// \brief Inserts or replaces the Component's Texture with the given channel.
    /// \param channel  Texture channel
    /// \param texture  New Texture.
    void set_texture(ushort channel, std::shared_ptr<Texture2> texture) { m_textures[channel] = texture; }

    /// \brief All Textures provided by this Component, indexed by channel.
    const std::unordered_map<ushort, std::shared_ptr<Texture2>>& all_textures() const { return m_textures; }

protected: // methods
    /// \brief Value Constructor.
    explicit TextureComponent() = default;

protected: // fields
    /// \brief Textures indexed by their channel.
    std::unordered_map<ushort, std::shared_ptr<Texture2>> m_textures;
};

} // namespace signal
