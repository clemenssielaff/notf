#include "app/property_graph.hpp"

#include "common/log.hpp"
#include "common/set.hpp"
#include "utils/type_name.hpp"

NOTF_OPEN_NAMESPACE

//====================================================================================================================//

PropertyGraph::lookup_error::~lookup_error() = default;

PropertyGraph::type_error::~type_error() = default;

PropertyGraph::cyclic_dependency_error::~cyclic_dependency_error() = default;

//====================================================================================================================//

PropertyGraph::PropertyBase::~PropertyBase() = default;

//====================================================================================================================//

void PropertyGraph::delete_property(const PropertyBase* property)
{
    if (!property) {
        return;
    }
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_properties.find(property);
    if (it == m_properties.end()) {
        return;
    }
    it->second->prepare_removal(*this);
    m_properties.erase(it);
}

void PropertyGraph::_detect_cycles(const PropertyBase* property, const std::vector<PropertyBase*>& dependencies)
{
    std::unordered_set<PropertyBase*> unchecked, checked;
    std::copy(dependencies.begin(), dependencies.end(), std::inserter(unchecked, unchecked.begin()));

    PropertyBase* candidate;
    while (pop_one(unchecked, candidate)) {
        if (property == candidate) {
            notf_throw(cyclic_dependency_error, "Failed to create property expression which would introduce a cyclic "
                                                "dependency");
        }
        checked.insert(candidate);
        PropertyBase* property = _find_property(candidate);
        for (PropertyBase* dependency : property->dependencies()) {
            if (!checked.count(dependency)) {
                unchecked.emplace(dependency);
            }
        }
    }
}

void PropertyGraph::_throw_not_found(const PropertyBase* property) const
{
    notf_throw_format(lookup_error, "Unknown Property \"" << property << "\"");
}

void PropertyGraph::_throw_wrong_type(const PropertyBase* property, const std::type_info& expected,
                                      const std::type_info& actual) const
{
    notf_throw_format(type_error, "Wrong property type requested of Property \""
                                      << property << "\""
                                      << "(\"" << type_name(actual) << " instead of \"" << type_name(expected)
                                      << "\")");
}

NOTF_CLOSE_NAMESPACE
