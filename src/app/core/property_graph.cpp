#include "app/core/property_graph.hpp"

#include "common/log.hpp"
#include "common/set.hpp"
#include "common/vector.hpp"

NOTF_OPEN_NAMESPACE

PropertyGraph::lookup_error::~lookup_error() = default;

PropertyGraph::type_error::~type_error() = default;

PropertyGraph::cyclic_dependency_error::~cyclic_dependency_error() = default;

//====================================================================================================================//

void PropertyGraph::Property::_clear_dependencies() noexcept
{
    for (Property* dependency : m_dependencies) {
        const bool should_always_succeed = remove_one_unordered(dependency->m_affected, this);
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

void PropertyGraph::Property::_clean_value(const PropertyKey& my_key, const PropertyGraph& graph)
{
    assert(m_value.has_value());
    if (m_is_dirty) {
        assert(m_expression);
        auto result = m_expression(graph);
        if (result.type() != m_value.type()) {
            _freeze();
            // TODO: test this out
            log_critical << "Expression for Property \"" << my_key << "\" returned wrong type (\""
                         << type_name(result.type()) << "\" instead of \"" << type_name(m_value.type())
                         << "\"). The expression has been disabled.";
        }
        m_value = std::move(result);
        m_is_dirty = false;
    }
}

//====================================================================================================================//

void PropertyGraph::delete_property(const PropertyKey& key)
{
    auto it = m_properties.find(key);
    if (it == m_properties.end()) {
        return;
    }
    it->second.prepare_removal();
    m_properties.erase(it);
}

const PropertyGraph::Property& PropertyGraph::_find_property(const PropertyKey& key) const
{
    if (key.property_id.is_valid()) { // you cannot request the implicit PropertyGroup properties
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

std::vector<PropertyGraph::Property*> PropertyGraph::_collect_properties(const std::vector<PropertyKey>& keys)
{
    std::vector<Property*> result;
    result.reserve(keys.size());

    std::vector<PropertyKey> broken_keys;

    // collect properties
    for (const PropertyKey& dependency_key : keys) {
        auto it = m_properties.find(dependency_key);
        if (it == m_properties.end()) {
            broken_keys.emplace_back(dependency_key);
        }
        else {
            result.emplace_back(&it->second);
        }
    }

    // in case one or more keys were invalid, generate a nice error message and throw
    if (!broken_keys.empty()) {
        std::stringstream ss;
        ss << "Cannot create expression with unkown propert" << (broken_keys.size() == 1 ? "y" : "ies") << ": ";
        for (size_t i = 0, end = broken_keys.size() - 1; i != end; ++i) {
            ss << broken_keys[i] << ", ";
        }
        ss << broken_keys.back();
        notf_throw(lookup_error, ss.str());
    }

    return result;
}

void PropertyGraph::_detect_cycles(const Property& property, const std::vector<Property*>& dependencies)
{
    std::unordered_set<const Property*> unchecked, checked;
    std::copy(dependencies.begin(), dependencies.end(), std::inserter(unchecked, unchecked.begin()));

    const Property* candidate;
    while (pop_one(unchecked, candidate)) {
        if (&property == candidate) {
            notf_throw(cyclic_dependency_error, "Failed to create property expression which would introduce a cyclic "
                                                "dependency");
        }
        checked.insert(candidate);
        for (const Property* dependency : candidate->dependencies()) {
            if (!checked.count(dependency)) {
                unchecked.emplace(dependency);
            }
        }
    }
}

NOTF_CLOSE_NAMESPACE
