#pragma once

#include <atomic>
#include <memory>

#ifdef _DEBUG
#include <type_traits>
#endif

#include "common/devel.hpp"

namespace signal {

/// \brief Base class for all Components.
///
class Component : public std::enable_shared_from_this<Component> {

    friend class Application;

public: // enum
    /// \brief Component kind enum.
    /// This enum controls a lot more than is immediately apparent.
    /// It acts as a unique identifier of each Component type, as index for the Widget components member
    /// and defines the order in which Components are updated by the Application.
    /// All of these uses are orthogonal, which is why we allow this kind of overloading.
    /// Just be sure you know what you are doing when you modify the enum.
    enum class KIND {
        SHAPE = 0, // first entry must be zero
        TEXTURE,
        _count, // must always be the last entry
    };

protected: // methods for Widget
    /// \brief Default Constructor.
    explicit Component();

public: // methods
    Component(Component const&) = delete;
    Component& operator=(Component const&) = delete;

    /// \brief Virtual Destructor.
    virtual ~Component();

    /// \brief This Component's type.
    virtual KIND get_kind() const = 0;

    /// \brief Updates this Component.
    virtual void update() = 0;

    /// \brief Checks whether this Component has been flaged as dirty.
    bool is_dirty() const { return m_is_dirty; }

    /// \brief Registers this Component to be updated by the Application before the next redraw.
    void set_dirty();

public: // static methods
    /// \brief The number of available components.
    static constexpr auto get_count() { return to_number(KIND::_count); }

private: // methods for Application
    /// \brief Lets the Component know that it has been cleaned.
    void set_clean() { m_is_dirty = false; }

private: // fields
    /// \brief Dirty flag for Components
    /// Is atomic, because it might be set from a worker thread and queried by the main thread at the same time.
    std::atomic_bool m_is_dirty;
};

namespace component_hpp {

// https://stackoverflow.com/a/8147213/3444217 and https://stackoverflow.com/a/25069711/3444217
template <class COMPONENT>
class MakeSharedEnabler : public COMPONENT {
public:
    template <typename... Args>
    MakeSharedEnabler(Args&&... args)
        : COMPONENT(std::forward<Args>(args)...)
    {
    }
};

} // namespace component_hpp

/// \brief Factory function to create shared pointers to any subclass of Component.
/// Make sure that all Component subclasses have a protected default Constructor
template <class COMPONENT, typename... ARGS>
std::shared_ptr<COMPONENT> make_component(ARGS&&... args)
{
#ifdef _DEBUG
    static_assert(std::is_base_of<Component, COMPONENT>::value,
        "make_component only works with subclasses of signal::Component");
#endif
    return std::make_shared<component_hpp::MakeSharedEnabler<COMPONENT>>(std::forward<ARGS>(args)...);
}

} // namespace signal
