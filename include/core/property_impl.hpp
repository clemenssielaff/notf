#pragma once

#include <iostream>

#include "common/claim.hpp"
#include "common/size2f.hpp"
#include "common/transform2.hpp"
#include "core/property.hpp"

namespace notf {

/**********************************************************************************************************************/

/* How to add a new Property - a checklist:
 *
 * In property_impl.hpp:
 *
 *  1. Add a PROPERTY_SPECIALIZATION with the name of the new Property subclass as first and the value type as
 *     second argument.
 *
 * In property_impl.cpp
 *
 *  2. Add NOTF_ADD_PROPERTY specializations for all types from which your new Property subclass can be contructed.
 */

#pragma push_macro("PROPERTY_SPECIALIZATION")
#define PROPERTY_SPECIALIZATION(NAME, TYPE)                          \
    class NAME : public Property<TYPE> {                             \
    public:                                                          \
        NAME(const TYPE value, const PropertyMap::iterator iterator) \
            : Property<TYPE>(std::move(value), std::move(iterator))  \
        {                                                            \
        }                                                            \
    };                                                               \
    template <>                                                      \
    PropertyMap::iterator add_property(PropertyMap& property_map, std::string name, const int value);

PROPERTY_SPECIALIZATION(BoolProperty, bool);
PROPERTY_SPECIALIZATION(FloatProperty, float);
PROPERTY_SPECIALIZATION(IntProperty, int);
PROPERTY_SPECIALIZATION(StringProperty, std::string);
PROPERTY_SPECIALIZATION(ClaimProperty, Claim);
PROPERTY_SPECIALIZATION(Size2Property, Size2f);
PROPERTY_SPECIALIZATION(Transform2Property, Transform2);

#undef PROPERTY_SPECIALIZATION
#pragma pop_macro("PROPERTY_SPECIALIZATION")

/**********************************************************************************************************************/

/** Prints the contents of a Property into a std::ostream.
 * @param out       Output stream, implicitly passed with the << operator.
 * @param property  Property to print.
 * @return          Output stream for further output.
 */
template <typename T>
std::ostream& operator<<(std::ostream& out, const Property<T>& property)
{
    return out << "Property \"" << property.get_name() << "\": " << property.get_value();
}

} // namespace notf
