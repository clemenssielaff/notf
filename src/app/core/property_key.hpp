#pragma once

#include "app/ids.hpp"
#include "utils/bitsizeof.hpp"

NOTF_OPEN_NAMESPACE

template<typename T>
struct TypedPropertyKey;

//====================================================================================================================//

/// A PropertyKey is used to identify a single Property in the PropertyGraph.
/// It consists of both the ID of the Property itself, as well as that of its Item.
struct PropertyKey {

    // methods -------------------------------------------------------------------------------------------------------//
public:
    /// No default constructor.
    PropertyKey() = delete;

    /// Value constructor.
    /// @param item_id      ID of the Item owning the Property.
    /// @param property_id  ID of the Property in the Item's PropertyGroup.
    PropertyKey(ItemId item_id, PropertyId property_id) : m_item_id(item_id), m_property_id(property_id) {}

    //    /// Copy constructor.
    //    /// @param other    PropertyKey to copy from.
    //    PropertyKey(const PropertyKey& other) : m_item_id(other.m_item_id), m_property_id(other.m_property_id) {}

    //    /// Move constructor.
    //    /// @param other    PropertyKey to copy from.
    //    PropertyKey(PropertyKey&& other) : m_item_id(other.m_item_id), m_property_id(other.m_property_id) {
    //        other.m_item_id = ItemId::invalid();
    //        other.m_property_id = ItemId::m_property_id();
    //    }

    //    /// Copy constructor from typed key.
    //    /// @param other    TypedPropertyKey to copy from.
    //    template<typename T>
    //    PropertyKey(const TypedPropertyKey<T>& other) : m_item_id(other.m_item_id), m_property_id(other.m_property_id)
    //    {}

    //    PropertyKey& operator=(const PropertyKey& other)
    //    {

    //    }

    /// ID of the Item owning the Property.
    ItemId item_id() const { return m_item_id; }

    /// ID of the Property in the Item's PropertyGroup.
    PropertyId property_id() const { return m_property_id; }

    /// Explicit invalid PropertyKey generator.
    static PropertyKey invalid() { return PropertyKey(ItemId::INVALID, PropertyId::INVALID); }

    /// Equality operator.
    /// @param rhs  PropertyKey to test against.
    bool operator==(const PropertyKey& rhs) const
    {
        return m_item_id == rhs.m_item_id && m_property_id == rhs.m_property_id;
    }

    /// Inequality operator.
    /// @param rhs  PropertyKey to test against.
    bool operator!=(const PropertyKey& rhs) const
    {
        return m_item_id != rhs.m_item_id || m_property_id != rhs.m_property_id;
    }

    /// Lesser-than operator.
    /// @param rhs  PropertyKey to test against.
    bool operator<(const PropertyKey& rhs) const
    {
        if (m_item_id < rhs.m_item_id) {
            return true;
        }
        else {
            return m_item_id == rhs.m_item_id && m_property_id < rhs.m_property_id;
        }
    }

    /// Lesser-or-equal-than operator.
    /// @param rhs  PropertyKey to test against.
    bool operator<=(const PropertyKey& rhs) const
    {
        if (m_item_id < rhs.m_item_id) {
            return true;
        }
        else {
            return m_item_id == rhs.m_item_id && m_property_id <= rhs.m_property_id;
        }
    }

    /// Greater-than operator.
    /// @param rhs  PropertyKey to test against.
    bool operator>(const PropertyKey& rhs) const
    {
        if (m_item_id > rhs.m_item_id) {
            return true;
        }
        else {
            return m_item_id == rhs.m_item_id && m_property_id > rhs.m_property_id;
        }
    }

    /// Greater-or-equal-than operator.
    /// @param rhs  PropertyKey to test against.
    bool operator>=(const PropertyKey& rhs) const
    {
        if (m_item_id > rhs.m_item_id) {
            return true;
        }
        else {
            return m_item_id == rhs.m_item_id && m_property_id >= rhs.m_property_id;
        }
    }

    /// Checks if this PropertyKey is valid or not.
    bool is_valid() const { return m_item_id != ItemId::INVALID && m_property_id != PropertyId::INVALID; }

    /// Checks if this PropertyKey is valid or not.
    explicit operator bool() const { return is_valid(); }

    // fields --------------------------------------------------------------------------------------------------------//
private:
    /// ID of the Item owning the Property.
    ItemId m_item_id;

    /// ID of the Property in the Item's PropertyGroup.
    PropertyId m_property_id;
};

//====================================================================================================================//

/// PropertyKey with an associated value type.
/// While the PropertyGraph stores untyped PropertyKeys, Items know what value type a Property is and can use that
/// knowledge to hide away casts from and to std::any.
template<typename T>
struct TypedPropertyKey : public PropertyKey {

    // types ---------------------------------------------------------------------------------------------------------//
public:
    /// Value type of the Property.
    using value_t = T;
};

//====================================================================================================================//

/// Prints a human-readable representation of a PropertyKey into a std::ostream.
/// @param os   Output stream, implicitly passed with the << operator.
/// @param key  PropertyKey to print.
/// @return Output stream for further output.
inline std::ostream& operator<<(std::ostream& out, const PropertyKey& key)
{
    return out << "(" << key.item_id() << ", " << key.property_id() << ")";
}

NOTF_CLOSE_NAMESPACE

//== std::hash =======================================================================================================//

namespace std {

template<>
struct hash<notf::PropertyKey> {
    size_t operator()(const notf::PropertyKey& key) const
    {
        return (static_cast<size_t>(key.item_id().value()) << (notf::bitsizeof<size_t>() / 2))
               ^ key.property_id().value();
    }
};

} // namespace std
