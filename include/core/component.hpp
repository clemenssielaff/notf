#pragma once

#include <memory>
#include <type_traits>

#include "common/devel.hpp"

namespace signal {

/// \brief Virtual base class for all Components.
///
class Component {

public: // enums
    /// \brief Component kind enum.
    ///
    /// Acts as a unique identifier of each Component type and as index for the Widget components member.
    enum class KIND {
        INVALID = 0,
        SHAPE,
        RENDER,
        COLOR,
        TEXTURE,
        _count, // must always be the last entry
    };

public: // methods
    Component(Component const&) = delete; // no copy construction
    Component& operator=(Component const&) = delete; // no copy assignment

    /// \brief Virtual Destructor.
    virtual ~Component();

    /// \brief Abstract method to validate a fully constructed component.
    /// \return True iff the Component is valid, false otherwise.
    /// Implement in subclasses (if you want) to perform specific checks.
    virtual bool is_valid() { return true; }

    /// \brief This Component's type.
    virtual KIND get_kind() const = 0;

protected: // methods
    /// \brief Default Constructor.
    explicit Component() = default;
};

/// \brief Factory function to create shared pointers to any subclass of Component.
/// Make sure that all Component subclasses have a protected Constructor
/// If the Component fails validation with `validate`, the returned shared pointer will be empty.
template <class COMPONENT, typename... ARGS>
std::shared_ptr<COMPONENT> make_component(ARGS&&... args)
{
    static_assert(std::is_base_of<Component, COMPONENT>::value,
                  "make_component must only be used with subclasses of signal::Component");
    auto component = std::make_shared<MakeSharedEnabler<COMPONENT>>(std::forward<ARGS>(args)...);
    if(!component->is_valid()){
        return {};
    }
    return component;
}

} // namespace signal
