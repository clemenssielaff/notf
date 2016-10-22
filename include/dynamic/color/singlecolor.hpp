#include "core/components/color_component.hpp"

namespace notf {

/// \brief A Color component providing a single color.
///
class SingleColor : public ColorComponent {

public: // methods
    /// \brief Returns the Component's single Color, regardless of the given index.
    virtual Color get_color(int) const override;

protected: // methods
    /// \brief Value Constructor.
    ///
    /// \param color    The single Color provided by this Component.
    explicit SingleColor(Color color)
        : ColorComponent()
        , m_color(std::move(color))
    {
    }

private: // fields
    /// \brief The single Color provided by this Component.
    Color m_color;
};

} // namespace notf
