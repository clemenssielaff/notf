#include "app/core/property_graph.hpp"

#include "common/log.hpp"
#include "common/set.hpp"
#include "common/vector.hpp"

namespace notf {

namespace detail {

PropertyBase::~PropertyBase() noexcept {}

void PropertyBase::clear_dependencies()
{
    for (PropertyBase* dependency : m_dependencies) {
        const bool should_always_succeed = remove_one_unordered(dependency->m_affected, this);
        assert(should_always_succeed);
    }
    m_dependencies.clear();
}

void PropertyBase::freeze_affected()
{
    for (PropertyBase* affected : m_affected) {
        affected->freeze();
    }
}

void PropertyBase::_register_with_dependencies()
{
    for (PropertyBase* dependency : m_dependencies) {
        dependency->m_affected.emplace_back(this);
    }
}

void PropertyBase::_set_affected_dirty()
{
    for (PropertyBase* affected : m_affected) {
        affected->m_is_dirty = true;
    }
}

} // namespace detail

bool PropertyGraph::delete_property(const Id id)
{
    auto it = m_properties.find(static_cast<id_t>(id));
    if (it == m_properties.end()) {
        return false;
    }

    PropertyBase* property = it->second.get();
    property->clear_dependencies();
    property->freeze_affected();

    m_properties.erase(it);
    return true;
}

PropertyGraph::id_t PropertyGraph::_next_id()
{
    id_t new_id;
    do {
        new_id = m_next_id++;
    } while (!new_id or m_properties.count(new_id));
    return new_id;
}

bool PropertyGraph::_get_properties(const std::vector<Id>& ids, std::vector<PropertyBase*>& result)
{
    result.reserve(ids.size());
    std::vector<id_t> broken_ids;

    // collect properties
    for (const Id& dependency_id : ids) {
        auto it = m_properties.find(static_cast<id_t>(dependency_id));
        if (it == m_properties.end()) {
            broken_ids.emplace_back(static_cast<id_t>(dependency_id));
        }
        else {
            result.emplace_back(it->second.get());
        }
    }
    if (broken_ids.empty()) {
        return true;
    }

    // in case one or more ids were invalid, generate a nice error message and return false
    std::stringstream ss;
    ss << "Cannot create expression with unkown propert" << (broken_ids.size() == 1 ? "y" : "ies") << ": ";
    for (size_t i = 0, end = broken_ids.size() - 1; i != end; ++i) {
        ss << broken_ids[i] << ", ";
    }
    ss << broken_ids.back();
    log_critical << ss.str();
    return false;
}

bool PropertyGraph::_is_dependency_of_any(const PropertyBase* property, const std::vector<PropertyBase*>& dependencies)
{
    std::set<const PropertyBase*> unchecked, checked;
    std::copy(dependencies.begin(), dependencies.end(), std::inserter(unchecked, unchecked.begin()));

    const PropertyBase* candidate;
    while (pop_one(unchecked, candidate)) {
        if (property == candidate) {
            log_critical << "Could not define expression: circular dependency detected";
            return true;
        }

        checked.insert(candidate);
        for (const PropertyBase* dependency : candidate->m_dependencies) {
            if (!checked.count(dependency)) {
                unchecked.emplace(dependency);
            }
        }
    }

    return false;
}

} // namespace notf
