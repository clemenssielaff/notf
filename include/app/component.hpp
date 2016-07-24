#pragma once

namespace signal {

class Component {

public: // enum
    ///
    /// \brief Component kind enum.
    ///
    enum class KIND {
        OPENGL,
        AABR,
        SHAPE,
        _count, // must always be the last entry
    };

public: // methods
    ///
    /// \brief Virtual Destructor.
    ///
    virtual ~Component();

    ///
    /// \brief This Component's type.
    ///
    virtual KIND get_kind() const = 0;
};

///
/// \brief The number of available components in this build.
///
constexpr int componentKindCount() { return static_cast<int>(Component::KIND::_count); }

} // namespace signal
