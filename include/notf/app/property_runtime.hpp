#pragma once

#include "notf/app/property.hpp"

NOTF_OPEN_NAMESPACE

// run time property ================================================================================================ //

template<class T>
class RunTimeProperty final : public Property<T> {

    // methods --------------------------------------------------------------------------------- //
public:
    /// Constructor.
    /// @param name         The Node-unique name of this Property.
    /// @param value        Property value.
    /// @param is_visible   Whether a change in the Property will cause the Node to redraw or not.
    RunTimeProperty(std::string_view name, T value, bool is_visible = true)
        : Property<T>(value, is_visible), m_name(std::move(name)), m_default_value(std::move(value))
    {}

    /// The Node-unique name of this Property.
    std::string_view get_name() const final { return m_name; }

    /// The default value of this Property.
    const T& get_default() const final { return m_default_value; }

    // fields ---------------------------------------------------------------------------------- ///
private:
    /// The Node-unique name of this Property.
    const std::string_view m_name;

    /// The default value of this Property.
    const T m_default_value;
};

NOTF_CLOSE_NAMESPACE
