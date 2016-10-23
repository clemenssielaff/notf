#pragma once

#include <memory>
#include <unordered_map>

#include "core/component.hpp"

namespace notf {

class Texture2;

using TextureChannels = std::unordered_map<ushort, std::shared_ptr<Texture2>>;

/// @brief A Texture Component contains 0-n Textures that are identifiable by their Texture Channel.
///
class TextureComponent : public Component {

public: // methods
    /// @brief This Component's type.
    virtual KIND get_kind() const override { return KIND::TEXTURE; }

    /// @brief Binds the OpenGL texture with the given channel.
    /// \returns True iff the texture was bound, false if the channel is empty.
    bool bind_texture(ushort channel) const;

    /// @brief Returns the Texture of the requested channel.
    /// @param channel  Texture channel
    /// @return The requested Texture, is empty if the Component doesn't provide the given channel.
    std::shared_ptr<Texture2> get_texture(ushort channel) const;

public: // methods
    /// @brief Value Constructor.
    explicit TextureComponent(std::unordered_map<ushort, std::shared_ptr<Texture2>> textures);

protected: // fields
    /// @brief Textures indexed by their channel.
    TextureChannels m_textures;
};

} // namespace notf
