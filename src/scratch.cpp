#include "core/property_impl.hpp"
using namespace notf;

#include <assert.h>

//int main() {
int notmain() {
    PropertyMap propMap;

    {
        PropertyMap::iterator it;
        bool success;
        std::tie(it, success) = propMap.emplace(std::make_pair<std::string, std::unique_ptr<AbstractProperty>>("dabool", {}));
        assert(success);
        it->second = std::make_unique<BoolProperty>(true, it);
    }
    {
        PropertyMap::iterator it;
        bool success;
        std::tie(it, success) = propMap.emplace(std::make_pair<std::string, std::unique_ptr<AbstractProperty>>("dabool2", {}));
        assert(success);
        it->second = std::make_unique<BoolProperty>(false, it);
    }

    auto it = propMap.find("dabool");
    if (it->second->get_type() == BoolProperty::Type::BOOL) {
        const auto& prop = static_cast<const BoolProperty&>(*it->second);
        std::cout << prop << std::endl;
    }

    it = propMap.find("dabool2");
    if (it->second->get_type() == BoolProperty::Type::BOOL) {
        const auto& prop = static_cast<const BoolProperty&>(*it->second);
        std::cout << prop << std::endl;
    }

    std::cout << "Size of iterator: " << sizeof(it) << std::endl;
    std::cout << "Size of double: " << sizeof(double) << std::endl;

    return 0;
}
