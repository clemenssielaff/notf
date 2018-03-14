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

void PropertyManager::Property::prepare_removal(const PropertyKey& key, PropertyManager& graph)
{
    _unregister_from_dependencies(key, graph);
    for (const PropertyKey& affectedKey : m_affected) {
        Property& affected = graph._find_property(affectedKey);
        affected._freeze(key, graph);
    }
}

void PropertyManager::Property::_evaluate_expression(const PropertyKey& key, PropertyManager& graph)
{
    assert(m_expression);
    auto result = m_expression(graph);
    if (result.type() != m_value.type()) {
        _freeze(key, graph);
        log_critical << "Expression for Property \"" << key << "\" returned wrong type (\"" << type_name(result.type())
                     << "\" instead of \"" << type_name(m_value.type())
                     << "\"). The expression has been disabled to avoid future errors.";
        return;
    }
    m_value = std::move(result);
    _set_clean();
}

void PropertyManager::Property::_register_with_dependencies(const PropertyKey& key, PropertyManager& graph)
{
    for (const PropertyKey& dependencyKey : m_dependencies) {
        Property& dependency = graph._find_property(dependencyKey);
        dependency.m_affected.emplace_back(key);
    }
}

void PropertyManager::Property::_unregister_from_dependencies(const PropertyKey& key, PropertyManager& graph)
{
    for (const PropertyKey& dependencyKey : m_dependencies) {
        Property& dependency = graph._find_property(dependencyKey);
        auto it = std::find(dependency.m_affected.begin(), dependency.m_affected.end(), key);
        assert(it != dependency.m_affected.end());
        *it = std::move(dependency.m_affected.back());
        dependency.m_affected.pop_back();
    }
    m_dependencies.clear();
}

void PropertyManager::Property::_set_affected_dirty(const PropertyKey& key, PropertyManager& graph)
{
    for (const PropertyKey& affectedKey : m_affected) {
        Property& affected = graph._find_property(affectedKey);
        affected._set_dirty(key, graph);
    }
}

void PropertyManager::Property::_assert_correct_type(const PropertyKey& key, const std::type_info& info)
{
    if (info != m_value.type()) {
        notf_throw_format(type_error, "Wrong property type requested of Property \""
                                          << key << "\""
                                          << "(\"" << type_name(info) << " instead of \"" << type_name(m_value.type())
                                          << "\")");
    }
}

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
    it->second.prepare_removal(key, *this);
    m_properties.erase(it);
}

void PropertyManager::_throw_notfound(const PropertyKey key)
{
    notf_throw_format(lookup_error, "Unknown Property \"" << key << "\"");
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
        Property& property = _find_property(candidate);
        for (const PropertyKey& dependency : property.dependencies()) {
            if (!checked.count(dependency)) {
                unchecked.emplace(dependency);
            }
        }
    }
}

NOTF_CLOSE_NAMESPACE
