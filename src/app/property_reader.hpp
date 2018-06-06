#pragma once

#include "app/forwards.hpp"

NOTF_OPEN_NAMESPACE

namespace access { // forwards
template<class>
class _PropertyReader;
} // namespace access

// ================================================================================================================== //

class PropertyReader {

    friend class access::_PropertyReader<PropertyBody>;

    // types -------------------------------------------------------------------------------------------------------- //
public:
    /// Access types.
    template<class T>
    using Access = access::_PropertyReader<T>;

    // methods ------------------------------------------------------------------------------------------------------ //
public:
    /// Value constructor.
    /// @param body     Owning pointer to the PropertyBody to read from.
    PropertyReader(PropertyBodyPtr&& body) : m_body(std::move(body)) {}

    /// Equality operator.
    /// @param rhs      Other reader to compare against.
    bool operator==(const PropertyReader& rhs) const { return m_body == rhs.m_body; }

    /// Checks whether this PropertyReader is valid or not.
    bool is_valid() const { return static_cast<bool>(m_body); }

    // fields ------------------------------------------------------------------------------------------------------- //
protected:
    /// Owning pointer to the PropertyBody to read from.
    PropertyBodyPtr m_body;
};

template<class T>
class TypedPropertyReader final : public PropertyReader {

    // methods ------------------------------------------------------------------------------------------------------ //
public:
    /// Value constructor.
    /// @param body     Owning pointer to the PropertyBody to read from.
    TypedPropertyReader(TypedPropertyBodyPtr<T>&& body) : PropertyReader(std::move(body)) {}

    /// Read-access to the value of the PropertyBody.
    const T& operator()() const { return static_cast<TypedPropertyBody<T>*>(m_body.get())->get(); }
};

// accessors -------------------------------------------------------------------------------------------------------- //

template<>
class access::_PropertyReader<PropertyBody> {
    friend class notf::PropertyBody;

    /// Mutex guarding all Property bodies.
    static const PropertyBodyPtr& property(const PropertyReader& reader) { return reader.m_body; }
};

// TODO: PropertyWriters

NOTF_CLOSE_NAMESPACE
