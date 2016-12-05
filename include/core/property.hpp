#pragma once

#include <map>
#include <memory>
#include <string>

#include "common/signal.hpp"

namespace notf {

class AbstractProperty;

using PropertyMap = std::map<std::string, std::unique_ptr<AbstractProperty>>;

/**********************************************************************************************************************/

/** An abstract Property.
 * Is pretty useless by itself, you'll have to cast it to a subclass of Property<T> to get any functionality out of it.
 */
class AbstractProperty : public Signaler<class AbstractProperty> {

public: // enum
    /** Type of this Property, corresponds to one of the Property<T> subclasses defined in property_impl.hpp . */
    enum class Type {
        BOOL,
        FLOAT,
        INT,
        STRING,
        CLAIM,
    };

public: // methods
    explicit AbstractProperty(const PropertyMap::iterator iterator)
        : m_it(iterator)
    {
    }

    virtual ~AbstractProperty();

    /** The type of this Property. */
    virtual Type get_type() const = 0;

    /** The name of this Property. */
    const std::string& get_name() const { return m_it->first; }

private: // fields
    /** Iterator to the item in the PropertyMap containing this Property.
     * Is valid as long as this Property exists.
     * Storing this is a lot cheaper than storing the name again.
     */
    const PropertyMap::iterator m_it;
};

/**********************************************************************************************************************/

/** A (templated) Property baseclass, used to derive subclasses in bulk (see property_impl.hpp for details). */
template <typename T>
class Property : public AbstractProperty {

public: // methods
    Property(const T value, const PropertyMap::iterator iterator)
        : AbstractProperty(std::move(iterator))
        , m_value(std::move(value))
    {
    }

    /** Returns the value of this Property. */
    const T& get_value() const { return m_value; }

    /** Updates the value of this Property. */
    void set_value(T value)
    {
        m_value = std::move(value);
        value_changed(m_value);
    }

public: // signals
    /** Emitted when the value of this Property has changed. */
    Signal<const T&> value_changed;

private: // fields
    /** Value of this Property. */
    T m_value;
};

/**********************************************************************************************************************/

/** Adds a new Property to the given PropertyMap.
 * @param property_map  Map to which to add the Property.
 * @param name          Name of the Property, must be unique in the map.
 * @param value         Value of the Property, must be of a type supported by AbstractProperty.
 * @return              Iterator to the new Property in the map.
 * @throw               std::runtime_error if the name is not unique.
 */
template <typename T>
PropertyMap::iterator add_property(PropertyMap& property_map, std::string name, const T value);

} // namespace notf
