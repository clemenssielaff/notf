#include "core/property_impl.hpp"

#include <type_traits>

#include "common/log.hpp"
#include "common/string_utils.hpp"

namespace { // anonymous

using namespace notf;

PropertyMap::iterator add_property_helper(PropertyMap& propertyMap, std::string name)
{
    PropertyMap::iterator it;
    bool is_unique;
    std::tie(it, is_unique) = propertyMap.emplace(std::make_pair<std::string, std::unique_ptr<AbstractProperty>>(std::move(name), {}));
    if (!is_unique) {
        const std::string message = string_format("Failed to add Property \"%s\" - the name is not unique.", it->first.c_str());
        log_critical << message;
        throw(std::runtime_error(std::move(message)));
    }
    return it;
}

} // namespace anonymous

namespace notf {

#define NOTF_ADD_PROPERTY(PROPERTY, TYPE)                                                             \
    template <>                                                                                       \
    PropertyMap::iterator add_property(PropertyMap& property_map, std::string name, const TYPE value) \
    {                                                                                                 \
        PropertyMap::iterator it = add_property_helper(property_map, std::move(name));                \
        it->second = std::make_unique<PROPERTY>(static_cast<TYPE>(value), it);                        \
        return it;                                                                                    \
    }

NOTF_ADD_PROPERTY(BoolProperty, bool);
NOTF_ADD_PROPERTY(FloatProperty, float);
NOTF_ADD_PROPERTY(FloatProperty, double);
NOTF_ADD_PROPERTY(IntProperty, int);
NOTF_ADD_PROPERTY(StringProperty, std::string);
NOTF_ADD_PROPERTY(ClaimProperty, Claim);
NOTF_ADD_PROPERTY(Size2Property, Size2f);
NOTF_ADD_PROPERTY(Transform2Property, Transform2);

} // namespace notf
