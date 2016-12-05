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

AbstractProperty::~AbstractProperty()
{
}

AbstractProperty::Type BoolProperty::get_type() const { return Type::BOOL; }
AbstractProperty::Type FloatProperty::get_type() const { return Type::FLOAT; }
AbstractProperty::Type IntProperty::get_type() const { return Type::INT; }
AbstractProperty::Type StringProperty::get_type() const { return Type::STRING; }
AbstractProperty::Type ClaimProperty::get_type() const { return Type::CLAIM; }

#define NOTF_ADD_PROPERTY(TYPE, PROPERTY)                                                             \
    template <>                                                                                       \
    PropertyMap::iterator add_property(PropertyMap& property_map, std::string name, const TYPE value) \
    {                                                                                                 \
        PropertyMap::iterator it = add_property_helper(property_map, std::move(name));                \
        it->second = std::make_unique<PROPERTY>(static_cast<TYPE>(value), it);                        \
        return it;                                                                                    \
    }

NOTF_ADD_PROPERTY(bool, BoolProperty);
NOTF_ADD_PROPERTY(float, FloatProperty);
NOTF_ADD_PROPERTY(double, FloatProperty);
NOTF_ADD_PROPERTY(int, IntProperty);
NOTF_ADD_PROPERTY(std::string, StringProperty);
NOTF_ADD_PROPERTY(Claim, ClaimProperty);

} // namespace notf
