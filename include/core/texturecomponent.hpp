#pragma once

#include "core/component.hpp"

namespace signal {

class TextureComponent : public Component {
public:
    explicit TextureComponent();

    /// \brief This Component's type.
    virtual KIND get_kind() const override { return KIND::TEXTURE; }
};

} // namespace signal
