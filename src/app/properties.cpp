#include "app/properties.hpp"

#include <algorithm>

#include "common/set.hpp"

NOTF_OPEN_NAMESPACE

//====================================================================================================================//

PropertyGraph::no_dag::~no_dag() = default;

PropertyGraph::no_graph::~no_graph() = default;

//====================================================================================================================//

PropertyGraph::PropertyBody::~PropertyBody()
{
    std::unique_lock<Mutex> lock(m_graph.m_mutex);
    _ground();
    for (valid_ptr<PropertyBody*> affected : m_downstream) {
        affected->_ground();
    }
}

void PropertyGraph::PropertyBody::_ground()
{
    NOTF_ASSERT(m_graph.m_mutex.is_locked_by_this_thread());
    for (valid_ptr<PropertyBody*> dependency : m_upstream) {
        auto it = std::find(dependency->m_downstream.begin(), dependency->m_downstream.end(), this);
        NOTF_ASSERT(it != dependency->m_downstream.end());

        *it = std::move(dependency->m_downstream.back());
        dependency->m_downstream.pop_back();
    }
    m_upstream.clear();
}

bool PropertyGraph::PropertyBody::_validate_upstream(const std::vector<valid_ptr<PropertyBody*>>& dependencies) const
{
    NOTF_ASSERT(m_graph.m_mutex.is_locked_by_this_thread());

    // fill the unordered set and
    std::set<valid_ptr<PropertyBody*>> unchecked;
    for (valid_ptr<PropertyBody*> dependency : dependencies) {
        // check if all properties are still alive
        if (!m_graph.m_properties.count(dependency)) {
            return false;
        }

        // check if this property is not already downstream of the dependency (should never happen)
        NOTF_ASSERT(std::find(dependency->m_downstream.begin(), dependency->m_downstream.end(), this)
                    != dependency->m_downstream.end());

        unchecked.insert(dependency);
    }

    // check for cyclic dependencies
    std::set<valid_ptr<PropertyBody*>> checked;
    risky_ptr<PropertyBody*> candidate;
    while (pop_one(unchecked, candidate)) {
        if (this == candidate) {
            notf_throw(no_dag, "Failed to create property expression which would introduce a cyclic dependency");
        }
        checked.insert(candidate);
        for (valid_ptr<PropertyBody*> dependency : candidate->m_upstream) {
            if (!checked.count(dependency)) {
                unchecked.emplace(dependency);
            }
        }
    }

    return true;
}

PropertyGraph::PropertyHead::~PropertyHead()
{
    if (auto property_graph = graph()) {
        property_graph->_delete_property(m_body);
    }
}

NOTF_CLOSE_NAMESPACE
