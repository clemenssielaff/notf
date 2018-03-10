#include "app/core/property_graph.hpp"

#include "common/log.hpp"
#include "common/set.hpp"
#include "common/vector.hpp"
#include "utils/type_name.hpp"

NOTF_OPEN_NAMESPACE

//====================================================================================================================//

PropertyGraph::lookup_error::~lookup_error() = default;

PropertyGraph::type_error::~type_error() = default;

PropertyGraph::cyclic_dependency_error::~cyclic_dependency_error() = default;

//====================================================================================================================//

void PropertyGraph::Property::add_member(PropertyKey key)
{
    assert(!m_value.has_value()); // group properties should not have a value
#ifdef NOTF_DEBUG
    assert(std::find(m_affected.begin(), m_affected.end(), key) == m_affected.end());
#endif
    m_affected.push_back(std::move(key));
}

void PropertyGraph::Property::remove_member(const PropertyKey& key) { remove_one_unordered(m_affected, key); }

void PropertyGraph::Property::_clear_dependencies(const PropertyKey& my_key, PropertyGraph& graph) noexcept
{
    for (const PropertyKey& dependencyKey : m_dependencies) {
        Property& dependency = graph._find_property(dependencyKey);
        const bool should_always_succeed = remove_one_unordered(dependency.m_affected, my_key);
        assert(should_always_succeed);
    }
    m_dependencies.clear();
}

void PropertyGraph::Property::_check_value_type(const PropertyKey& my_key, const std::type_info& info)
{
    assert(m_value.has_value());
    if (info != m_value.type()) {
        notf_throw_format(type_error, "Wrong property type requested of Property \""
                                          << my_key << "\""
                                          << "(\"" << type_name(info) << " instead of \"" << type_name(m_value.type())
                                          << "\")");
    }
}

void PropertyGraph::Property::_clean_value(const PropertyKey& my_key, PropertyGraph& graph)
{
    if (m_is_dirty) {
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
}

//====================================================================================================================//

void PropertyGraph::_delete_property(const PropertyKey key)
{
    if (!key.property_id().is_valid()) {
        return;
    }
    std::lock_guard<std::mutex> lock(m_mutex);

    auto propertyIt = m_properties.find(key);
    if (propertyIt == m_properties.end()) {
        return;
    }
    propertyIt->second.prepare_removal(key, *this);
    m_properties.erase(propertyIt);

    PropertyKey group_key = PropertyKey(key.item_id(), 0);
    auto groupIt = m_properties.find(group_key);
    assert(groupIt != m_properties.end());
    groupIt->second.remove_member(key);
}

void PropertyGraph::_delete_group(const ItemId id)
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

PropertyGraph::Property& PropertyGraph::_find_property(const PropertyKey key)
{
    if (key.property_id().is_valid()) { // you cannot request the implicit PropertyGroup properties
        const auto it = m_properties.find(key);
        if (it != m_properties.end()) {
            return it->second;
        }
    }
    notf_throw_format(lookup_error, "Unknown Property \"" << key << "\"");
}

PropertyGraph::Property& PropertyGraph::_group_for(const ItemId item_id)
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

void PropertyGraph::_detect_cycles(const PropertyKey key, const std::vector<PropertyKey>& dependencies)
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
