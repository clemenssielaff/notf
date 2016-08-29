#pragma once

#include "core/components/render_component.hpp"

namespace signal {

class SpriteComponent : public RenderComponent {

public: // methods
    /// \brief Renders the given Widget.
    /// \param widget   Widget to render.
    ///
    virtual void render(Widget& widget) override;

protected: // methods
    /// \brief Default Constructor.
    explicit SpriteComponent() = default;
};

} // namespace signal
