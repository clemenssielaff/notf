#include "app/property_manager.hpp"

#include "common/log.hpp"
#include "common/set.hpp"
#include "common/vector.hpp"
#include "utils/type_name.hpp"

NOTF_OPEN_NAMESPACE

//====================================================================================================================//

PropertyManager::lookup_error::~lookup_error() = default;

PropertyManager::type_error::~type_error() = default;

PropertyManager::cyclic_dependency_error::~cyclic_dependency_error() = default;

//====================================================================================================================//

void PropertyManager::Property::add_member(PropertyKey key)
{
    assert(!m_value.has_value()); // group properties should not have a value
#ifdef NOTF_DEBUG
    assert(std::find(m_affected.begin(), m_affected.end(), key) == m_affected.end());
#endif
    m_affected.push_back(std::move(key));
}

void PropertyManager::Property::remove_member(const PropertyKey& key)
{
    assert(!m_value.has_value()); // group properties should not have a value
    remove_one_unordered(m_affected, key);
}

void PropertyManager::Property::prepare_removal(const PropertyKey& my_key, PropertyManager& graph)
{
    _unregister_from_dependencies(my_key, graph);
    for (const PropertyKey& affectedKey : m_affected) {
        Property& affected = graph._find_property(affectedKey);
        affected._freeze(my_key, graph);
    }
}

void PropertyManager::Property::_evaluate_expression(const PropertyKey& my_key, PropertyManager& graph)
{
    assert(m_expression);
    auto result = m_expression(graph);
    if (result.type() != m_value.type()) {
        _freeze(my_key, graph);
        log_critical << "Expression for Property \"" << my_key << "\" returned wrong type (\""
                     << type_name(result.type()) << "\" instead of \"" << type_name(m_value.type())
                     << "\"). The expression has been disabled to avoid future errors.";
        return;
    }
    m_value = std::move(result);
    m_is_dirty = false;
}

void PropertyManager::Property::_register_with_dependencies(const PropertyKey& my_key, PropertyManager& graph)
{
    for (const PropertyKey& dependencyKey : m_dependencies) {
        Property& dependency = graph._find_property(dependencyKey);
        dependency.m_affected.emplace_back(my_key);
    }
}

void PropertyManager::Property::_unregister_from_dependencies(const PropertyKey& my_key, PropertyManager& graph)
{
    for (const PropertyKey& dependencyKey : m_dependencies) {
        Property& dependency = graph._find_property(dependencyKey);
        auto it = std::find(dependency.m_affected.begin(), dependency.m_affected.end(), my_key);
        assert(it != dependency.m_affected.end());
        *it = std::move(dependency.m_affected.back());
        dependency.m_affected.pop_back();
    }
    m_dependencies.clear();
}

void PropertyManager::Property::_set_affected_dirty(PropertyManager& graph)
{
    for (const PropertyKey& affectedKey : m_affected) {
        Property& affected = graph._find_property(affectedKey);
        affected.m_is_dirty = true;
    }
}

void PropertyManager::Property::_assert_correct_type(const PropertyKey& my_key, const std::type_info& info)
{
    if (info != m_value.type()) {
        notf_throw_format(type_error, "Wrong property type requested of Property \""
                                          << my_key << "\""
                                          << "(\"" << type_name(info) << " instead of \"" << type_name(m_value.type())
                                          << "\")");
    }
}

void PropertyManager::_delete_property(const PropertyKey key)
{
    if (!key.property_id().is_valid()) {
        return;
    }
    std::lock_guard<std::mutex> lock(m_mutex);

    { // remove property
        auto propertyIt = m_properties.find(key);
        if (propertyIt == m_properties.end()) {
            return;
        }
        propertyIt->second.prepare_removal(key, *this);
        m_properties.erase(propertyIt);
    }

    { // remove from group
        PropertyKey group_key = PropertyKey(key.item_id(), 0);
        auto groupIt = m_properties.find(group_key);
        assert(groupIt != m_properties.end());
        groupIt->second.remove_member(key);
    }
}

void PropertyManager::_delete_group(const ItemId id)
{
    if (!id.is_valid()) {
        return;
    }
    std::lock_guard<std::mutex> lock(m_mutex);

    PropertyKey key = PropertyKey(id, 0);
    auto groupIt = m_properties.find(key);
    if (groupIt == m_properties.end()) {
        return;
    }

    for (const PropertyKey& propertyKey : groupIt->second.members()) {
        auto propertyIt = m_properties.find(propertyKey);
        assert(propertyIt != m_properties.end());
        propertyIt->second.prepare_removal(propertyKey, *this);
        m_properties.erase(propertyIt);
    }
    m_properties.erase(groupIt);
}

void PropertyManager::_throw_notfound(const PropertyKey key)
{
    notf_throw_format(lookup_error, "Unknown Property \"" << key << "\"");
}

PropertyManager::Property& PropertyManager::_group_for(const ItemId item_id)
{
    assert(item_id.is_valid());
    PropertyKey key = PropertyKey(item_id, 0);
    auto it = m_properties.find(key);
    if (it != m_properties.end()) {
        return it->second;
    }
    auto result = m_properties.emplace(std::make_pair(std::move(key), Property()));
    assert(result.second);
    return result.first->second;
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
