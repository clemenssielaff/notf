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

PropertyGraph::group_error::~group_error() = default;

//====================================================================================================================//

void PropertyGraph::PropertyGroup::add_member(PropertyKey key)
{
#ifdef NOTF_DEBUG
    assert(std::find(m_members.begin(), m_members.end(), key) == m_members.end());
#endif
    m_members.push_back(std::move(key));
}

void PropertyGraph::PropertyGroup::remove_member(const PropertyKey& key) { remove_one_unordered(m_members, key); }

//====================================================================================================================//

void PropertyGraph::Property::prepare_removal(const PropertyKey& my_key, PropertyGraph& graph)
{
    _unregister_from_dependencies(my_key, graph);
    for (const PropertyKey& affectedKey : m_affected) {
        Property& affected = graph._find_property(affectedKey);
        affected._freeze(my_key, graph);
    }
}

void PropertyGraph::Property::_evaluate_expression(const PropertyKey& my_key, PropertyGraph& graph)
{
    assert(m_expression);
    auto result = m_expression(graph);
    if (result.type() != m_value.type()) {
        _freeze(my_key, graph);
        log_critical << "Expression for Property \"" << my_key << "\" returned wrong type (\""
                     << type_name(result.type()) << "\" instead of \"" << type_name(m_value.type())
                     << "\"). The expression has been disabled to avoid future errors.";
    }
    m_value = std::move(result);
    m_is_dirty = false;
}

void PropertyGraph::Property::_register_with_dependencies(const PropertyKey& my_key, PropertyGraph& graph)
{
    for (const PropertyKey& dependencyKey : m_dependencies) {
        Property& dependency = graph._find_property(dependencyKey);
        dependency.m_affected.emplace_back(my_key);
    }
}

void PropertyGraph::Property::_unregister_from_dependencies(const PropertyKey& my_key, PropertyGraph& graph)
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

void PropertyGraph::Property::_set_affected_dirty(PropertyGraph& graph)
{
    for (const PropertyKey& affectedKey : m_affected) {
        Property& affected = graph._find_property(affectedKey);
        affected.m_is_dirty = true;
    }
}

void PropertyGraph::Property::_assert_correct_type(const PropertyKey& my_key, const std::type_info& info)
{
    if (info != m_value.type()) {
        notf_throw_format(type_error, "Wrong property type requested of Property \""
                                          << my_key << "\""
                                          << "(\"" << type_name(info) << " instead of \"" << type_name(m_value.type())
                                          << "\")");
    }
}

void PropertyGraph::_add_group(const ItemId item_id, GraphicsProducerId producer)
{
    if (!item_id.is_valid()) {
        return;
    }
    const PropertyKey group_key = PropertyKey(item_id, 0);
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (m_properties.find(group_key) != m_properties.end()) {
            notf_throw_format(group_error, "Item #" << item_id << " already has an associated PropertyGroup");
        }
        auto result = m_properties.emplace(std::make_pair(group_key, PropertyGroup(producer)));
        assert(result.second);
    }
}

void PropertyGraph::_delete_property(const PropertyKey key)
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
        assert(std::holds_alternative<Property>(propertyIt->second));
        std::get<Property>(propertyIt->second).prepare_removal(key, *this);
        m_properties.erase(propertyIt);
    }

    { // remove from group
        PropertyKey group_key = PropertyKey(key.item_id(), 0);
        auto groupIt = m_properties.find(group_key);
        assert(groupIt != m_properties.end());
        assert(std::holds_alternative<PropertyGroup>(groupIt->second));
        PropertyGroup& group = std::get<PropertyGroup>(groupIt->second);
        group.remove_member(key);
    }
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
    assert(std::holds_alternative<PropertyGroup>(groupIt->second));
    PropertyGroup& group = std::get<PropertyGroup>(groupIt->second);

    for (const PropertyKey& propertyKey : group.members()) {
        auto propertyIt = m_properties.find(propertyKey);
        assert(propertyIt != m_properties.end());
        assert(std::holds_alternative<Property>(propertyIt->second));
        std::get<Property>(propertyIt->second).prepare_removal(propertyKey, *this);
        m_properties.erase(propertyIt);
    }
    m_properties.erase(groupIt);
}

void PropertyGraph::_throw_notfound(const PropertyKey key)
{
    notf_throw_format(lookup_error, "Unknown Property \"" << key << "\"");
}

PropertyGraph::PropertyGroup& PropertyGraph::_group_for(const ItemId item_id)
{
    assert(item_id.is_valid());
    PropertyKey key = PropertyKey(item_id, 0);
    auto it = m_properties.find(key);
    if (it != m_properties.end()) {
        assert(std::holds_alternative<PropertyGroup>(it->second));
        return std::get<PropertyGroup>(it->second);
    }
    notf_throw_format(group_error, "Item #" << item_id << " doesn't have an associated PropertyGroup");
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
