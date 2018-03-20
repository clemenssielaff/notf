#include "app/property_graph.hpp"

#include "common/log.hpp"
#include "common/set.hpp"

NOTF_OPEN_NAMESPACE

//====================================================================================================================//

PropertyGraph::no_dag_error::~no_dag_error() = default;

//====================================================================================================================//

PropertyGraph::PropertyNodeBase::~PropertyNodeBase()
{
    _unregister_from_dependencies();
    for (PropertyNodeBase* affected : m_affected) {
        affected->_freeze();
    }
}

void PropertyGraph::PropertyNodeBase::_detect_cycles(const std::vector<PropertyNodeBase*>& dependencies) const
{
    std::unordered_set<PropertyNodeBase*> unchecked, checked;
    std::copy(dependencies.begin(), dependencies.end(), std::inserter(unchecked, unchecked.begin()));

    PropertyNodeBase* candidate;
    while (pop_one(unchecked, candidate)) {
        if (this == candidate) {
            notf_throw(no_dag_error, "Failed to create property expression which would introduce a cyclic "
                                     "dependency");
        }
        checked.insert(candidate);
        for (PropertyNodeBase* dependency : candidate->m_dependencies) {
            if (!checked.count(dependency)) {
                unchecked.emplace(dependency);
            }
        }
    }
}

//====================================================================================================================//

void PropertyGraph::create_delta()
{

}


void PropertyGraph::resolve_delta()
{

}

NOTF_CLOSE_NAMESPACE
