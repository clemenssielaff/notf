#pragma once

#include <memory>
#include <set>
#include <type_traits>

#include "common/handle.hpp"
#include "common/signal.hpp"
#include "utils/smart_enabler.hpp"

namespace notf {

class Widget;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/// @brief Virtual base class for all Components.
class Component : public std::enable_shared_from_this<Component>, public Signaler<Component> {

    friend class Widget;

public: // enums
    /// @brief Component kind enum.
    ///
    /// Acts as a unique identifier of each Component type and as index for the Widget components member.
    enum class KIND : unsigned char {
        INVALID = 0,
        CANVAS,
        SHAPE,
        _count, // must always be the last entry
    };

public: // methods
    Component(const Component&) = delete; // no copy construction
    Component& operator=(const Component&) = delete; // no copy assignment

    /// @brief Virtual Destructor.
    virtual ~Component() = default;

    /// @brief Abstract method to validate a fully constructed component.
    /// @return True iff the Component is valid, false otherwise.
    /// Implement in subclasses (if you want) to perform specific checks.
    virtual bool is_valid() { return true; }

    /// @brief This Component's type.
    virtual KIND get_kind() const = 0;

protected: // methods
    /// @brief Default Constructor.
    explicit Component() = default;

    /// @brief Redraws all Widgets registered with this Component.
    void _redraw_widgets();

private: // methods for Widget
    // @brief Registers a new Widget to receive updates when this Component changes.
    void _register_widget(Handle widget_handle);

    /// @brief Unregisters a Widget from receiving updates from this Component.
    void _unregister_widget(Handle widget_handle);

private: // fields
    /// @brief Handles of all Widgets that use this Component.
    std::set<Handle> m_widgets;
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

class ShapeComponent;

/// @brief Returns the Component kind associated with a given Component subclass.
/// There should be one class name per entry in Component::KIND (order doesn't matter).
/// This is required by Widget::get_component<COMPONENT>() to correctly associate any Component subclass with its first-
/// level specialization.
template <typename T>
constexpr Component::KIND get_kind()
{
    Component::KIND kind = Component::KIND::INVALID;
    if (std::is_base_of<ShapeComponent, T>::value) {
        kind = Component::KIND::SHAPE;
    }
    return kind;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/// @brief Factory function to create shared pointers to any subclass of Component.
/// Make sure that all Component subclasses have a protected Constructor
/// @throw std::runtime_error   If the Component failed to validate with `validate`.
template <class COMPONENT, typename... ARGS>
std::shared_ptr<COMPONENT> make_component(ARGS&&... args)
{
    static_assert(std::is_base_of<Component, COMPONENT>::value,
                  "make_component must only be used with subclasses of notf::Component");
    auto component = std::make_shared<MakeSmartEnabler<COMPONENT>>(std::forward<ARGS>(args)...);
    if (!component->is_valid()) {
        throw std::runtime_error("Failed to produce valid Component");
    }
    return component;
}

} // namespace notf
