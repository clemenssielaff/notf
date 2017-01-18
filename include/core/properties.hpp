#pragma once

#include <iostream>

#include "common/claim.hpp"
#include "common/size2f.hpp"
#include "common/transform2.hpp"
#include "core/property.hpp"
#include "utils/make_smart_enabler.hpp"

namespace notf {

/**********************************************************************************************************************/

/* In order to add a new Property class, add a new DEFINE_PROPERTY and one or more CREATE_PROPERTY_IMPL (see below). */

#pragma push_macro("DEFINE_PROPERTY")

#define _notf_DEFINE_PROPERTY(NAME, TYPE)                            \
    class NAME : public Property<TYPE> {                             \
    protected:                                                       \
        NAME(const TYPE value, const PropertyMap::iterator iterator) \
            : Property<TYPE>(std::move(value), std::move(iterator))  \
        {                                                            \
        }                                                            \
                                                                     \
    public:                                                          \
        virtual ~NAME() override;                                    \
        virtual const std::string& get_type() const override         \
        {                                                            \
            static const std::string type_name = #NAME;              \
            return type_name;                                        \
        }                                                            \
        NAME& operator=(const TYPE value)                            \
        {                                                            \
            _set_value(std::move(value));                            \
            return *this;                                            \
        }                                                            \
        operator TYPE() const { return get_value(); }                \
    };

#ifdef NOTF_PROPERTY_IMPL
#define DEFINE_PROPERTY(NAME, TYPE)                                       \
    _notf_DEFINE_PROPERTY(NAME, TYPE);                                    \
    NAME::~NAME() {}                                                      \
    template <>                                                           \
    NAME* PropertyMap::get<NAME>(const std::string& name) const           \
    {                                                                     \
        auto value = at(name).get();                                      \
        auto result = dynamic_cast<NAME*>(value);                         \
        if (!result) {                                                    \
            throw std::runtime_error(string_format(                       \
                "Requested wrong type \"" #NAME "\" for Property \"%s\" " \
                "which is of type \"%s\"",                                \
                name.c_str(), value->get_type().c_str()));                \
        }                                                                 \
        return result;                                                    \
    }
#else
#define DEFINE_PROPERTY(NAME, TYPE)    \
    _notf_DEFINE_PROPERTY(NAME, TYPE); \
    template <>                        \
    NAME* PropertyMap::get<NAME>(const std::string& name) const;
#endif

DEFINE_PROPERTY(BoolProperty, bool);
DEFINE_PROPERTY(FloatProperty, float);
DEFINE_PROPERTY(IntProperty, int);
DEFINE_PROPERTY(StringProperty, std::string);
DEFINE_PROPERTY(ClaimProperty, Claim);
DEFINE_PROPERTY(Size2Property, Size2f);
DEFINE_PROPERTY(Transform2Property, Transform2);

#undef _notf_DEFINE_PROPERTY
#undef DEFINE_PROPERTY
#pragma pop_macro("DEFINE_PROPERTY")

/**********************************************************************************************************************/
#pragma push_macro("CREATE_PROPERTY_IMPL")

#ifdef NOTF_PROPERTY_IMPL
#define CREATE_PROPERTY_IMPL(NAME, TYPE)                                                                                             \
    template <>                                                                                                                      \
    NAME* PropertyMap::create_property(std::string name, const TYPE value)                                                           \
    {                                                                                                                                \
        iterator it;                                                                                                                 \
        bool is_unique;                                                                                                              \
        std::tie(it, is_unique) = emplace(std::make_pair<std::string, std::unique_ptr<AbstractProperty>>(std::move(name), {}));      \
        if (!is_unique) {                                                                                                            \
            const std::string message = string_format("Failed to add Property \"%s\" - the name is not unique.", it->first.c_str()); \
            log_critical << message;                                                                                                 \
            throw(std::runtime_error(std::move(message)));                                                                           \
        }                                                                                                                            \
        it->second = std::make_unique<MakeSmartEnabler<NAME>>(static_cast<TYPE>(value), it);                                         \
        return reinterpret_cast<NAME*>(it->second.get());                                                                            \
    }
#else
#define CREATE_PROPERTY_IMPL(NAME, TYPE) \
    template <>                          \
    NAME* PropertyMap::create_property(std::string name, const TYPE value);
#endif

CREATE_PROPERTY_IMPL(BoolProperty, bool);
CREATE_PROPERTY_IMPL(FloatProperty, float);
CREATE_PROPERTY_IMPL(FloatProperty, double);
CREATE_PROPERTY_IMPL(IntProperty, int);
CREATE_PROPERTY_IMPL(StringProperty, std::string);
CREATE_PROPERTY_IMPL(ClaimProperty, Claim);
CREATE_PROPERTY_IMPL(Size2Property, Size2f);
CREATE_PROPERTY_IMPL(Transform2Property, Transform2);

#undef CREATE_PROPERTY_IMPL
#pragma pop_macro("CREATE_PROPERTY_IMPL")

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
