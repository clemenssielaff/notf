#include "app/property_manager.hpp"

#include "common/log.hpp"
#include "common/set.hpp"
#include "utils/type_name.hpp"

NOTF_OPEN_NAMESPACE

//====================================================================================================================//

PropertyManager::lookup_error::~lookup_error() = default;

PropertyManager::type_error::~type_error() = default;

PropertyManager::cyclic_dependency_error::~cyclic_dependency_error() = default;

//====================================================================================================================//

PropertyManager::PropertyBase::~PropertyBase() = default;

//====================================================================================================================//

void PropertyManager::_delete_property(const PropertyKey key)
{
    if (!key.property_id().is_valid()) {
        return;
    }
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_properties.find(key);
    if (it == m_properties.end()) {
        return;
    }
    it->second->prepare_removal(key, *this);
    m_properties.erase(it);
}

void PropertyManager::_detect_cycles(const PropertyKey key, const std::vector<PropertyKey>& dependencies)
{
    std::unordered_set<PropertyKey> unchecked, checked;
    std::copy(dependencies.begin(), dependencies.end(), std::inserter(unchecked, unchecked.begin()));

    PropertyKey candidate = PropertyKey::invalid();
    while (pop_one(unchecked, candidate)) {
        if (key == candidate) {
            notf_throw(cyclic_dependency_error, "Failed to create property expression which would introduce a cyclic "
                                                "dependency");
        }
        checked.insert(candidate);
        PropertyBase* property = _find_property(candidate);
        for (const PropertyKey& dependency : property->dependencies()) {
            if (!checked.count(dependency)) {
                unchecked.emplace(dependency);
            }
        }
    }
}

void PropertyManager::_throw_not_found(const PropertyKey key)
{
    notf_throw_format(lookup_error, "Unknown Property \"" << key << "\"");
}

void PropertyManager::_throw_wrong_type(const PropertyKey key, const std::type_info& expected,
                                        const std::type_info& actual)
{
    notf_throw_format(type_error, "Wrong property type requested of Property \""
                                      << key << "\""
                                      << "(\"" << type_name(actual) << " instead of \"" << type_name(expected)
                                      << "\")");
}

NOTF_CLOSE_NAMESPACE
