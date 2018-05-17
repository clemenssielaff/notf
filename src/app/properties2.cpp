#include "app/properties2.hpp"

#include <algorithm>

#include "common/set.hpp"

NOTF_OPEN_NAMESPACE

namespace temp {

//====================================================================================================================//

PropertyGraph::no_dag::~no_dag() = default;

//====================================================================================================================//

PropertyGraph::Update::~Update() = default;

//====================================================================================================================//

void PropertyGraph::Batch::execute()
{
    //    PropertyGraph::UpdateSet affected;
    //    {
    //        NOTF_MUTEX_GUARD(PropertyGraph::s_mutex);

    //        // verify that all updates will succeed first
    //        for (auto& update : m_updates) {
    //            update->property->_validate_update(update.get());
    //        }

    //        // apply the updates
    //        for (auto& update : m_updates) {
    //            update->property->_apply_update(*property_graph, update.get(), affected);
    //        }
    //    }

    //    // fire off the combined event
    //    property_graph->_fire_event(std::move(affected));

    //    // reset in case the user wants to reuse the batch object
    //    m_updates.clear();
}

//====================================================================================================================//

PropertyGraph::PropertyBodyBase::~PropertyBodyBase() { PropertyBodyBase::_ground(); }

void PropertyGraph::PropertyBodyBase::_ground()
{
    NOTF_ASSERT(PropertyGraph::s_mutex.is_locked_by_this_thread());

    for (const PropertyAccessorBase& accessor : m_upstream) {
        PropertyBodyBase* dependency = accessor.m_body.get();
        auto it = std::find(dependency->m_downstream.begin(), dependency->m_downstream.end(), this);
        NOTF_ASSERT(it != dependency->m_downstream.end());
        *it = std::move(dependency->m_downstream.back());
        dependency->m_downstream.pop_back();
    }
    m_upstream.clear();
}

void PropertyGraph::PropertyBodyBase::_test_upstream(const Dependencies& dependencies)
{
    NOTF_ASSERT(PropertyGraph::s_mutex.is_locked_by_this_thread());

    std::set<valid_ptr<PropertyBodyBase*>> unchecked;
    for (const PropertyAccessorBase& accessor : dependencies) {
        if (PropertyBodyBase* dependency = accessor.m_body.get()) {
            unchecked.insert(dependency);
        }
    }

    std::set<valid_ptr<PropertyBodyBase*>> checked;
    PropertyBodyBase* candidate;
    while (pop_one(unchecked, candidate)) {
        if (this == candidate) {
            notf_throw(no_dag, "Failed to create property expression which would introduce a cyclic dependency");
        }
        checked.insert(candidate);
        for (const PropertyAccessorBase& accessor : candidate->m_upstream) {
            PropertyBodyBase* dependency = accessor.m_body.get();
            if (!checked.count(dependency)) {
                unchecked.emplace(dependency);
            }
        }
    }
}

void PropertyGraph::PropertyBodyBase::_set_upstream(Dependencies&& dependencies)
{
    NOTF_ASSERT(PropertyGraph::s_mutex.is_locked_by_this_thread());

#ifdef NOTF_DEBUG
    _test_upstream(dependencies); // input should already have been tested at this point, but better safe than sorry
#endif

    // remove potential duplicates in the input
    m_upstream.clear();
    m_upstream.reserve(dependencies.size());
    for (PropertyAccessorBase& accessor : dependencies) {
        if (std::find(m_upstream.begin(), m_upstream.end(), accessor) == m_upstream.end()) {
            m_upstream.emplace_back(std::move(accessor));
        }
    }

    // register with the upstream properties
    for (const PropertyAccessorBase& accessor : m_upstream) {
        PropertyBodyBase* dependency = accessor.m_body.get();
        dependency->_add_downstream(this);
    }
}

void PropertyGraph::PropertyBodyBase::_add_downstream(const valid_ptr<PropertyBodyBase*> affected)
{
    NOTF_ASSERT(PropertyGraph::s_mutex.is_locked_by_this_thread());

#ifdef NOTF_DEBUG
    auto it = std::find(m_downstream.begin(), m_downstream.end(), affected);
    NOTF_ASSERT(it == m_downstream.end()); // there should not be a way to register the same property twice
#endif
    m_downstream.emplace_back(affected);
}

//====================================================================================================================//

RecursiveMutex PropertyGraph::s_mutex = {};

} // namespace temp

NOTF_CLOSE_NAMESPACE
