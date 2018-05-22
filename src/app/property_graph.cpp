#include "app/property_graph.hpp"

#include <algorithm>
#include <set>

#include "common/set.hpp"

NOTF_OPEN_NAMESPACE

//====================================================================================================================//

PropertyGraph::no_dag_error::~no_dag_error() = default;

RecursiveMutex PropertyGraph::s_mutex = {};

#ifdef NOTF_TEST
std::atomic_size_t PropertyGraph::s_property_count{0};
#endif

//====================================================================================================================//

PropertyUpdate::~PropertyUpdate() = default;

//====================================================================================================================//

void PropertyBatch::execute()
{
    PropertyUpdateList effect;
    {
        NOTF_MUTEX_GUARD(PropertyGraph::Access<PropertyBatch>::mutex());

        // verify that all updates will succeed first
        for (const PropertyUpdatePtr& update : m_updates) {
            PropertyBodyBase::Access<PropertyBatch>::validate_update(update->property, update);
        }

        // apply the updates
        for (const PropertyUpdatePtr& update : m_updates) {
            PropertyBodyBase::Access<PropertyBatch>::apply_update(update->property, update, effect);
        }
    }

    // fire off the combined event
    //    property_graph->_fire_event(std::move(affected));
    // TODO: fire event

    // reset in case the user wants to reuse the batch object
    m_updates.clear();
}

//====================================================================================================================//

PropertyBodyBase::~PropertyBodyBase()
{
    PropertyBodyBase::_ground();

#ifdef NOTF_TEST
    --PropertyGraph::Access<PropertyBodyBase>::property_count();
#endif
}

void PropertyBodyBase::_ground()
{
    NOTF_ASSERT(_mutex().is_locked_by_this_thread());

    for (const PropertyReaderBase& reader : m_upstream) {
        PropertyBodyBase* dependency = PropertyReaderBase::PropertyBodyAccess::property(reader).get();
        auto it = std::find(dependency->m_downstream.begin(), dependency->m_downstream.end(), this);
        NOTF_ASSERT(it != dependency->m_downstream.end());
        *it = std::move(dependency->m_downstream.back());
        dependency->m_downstream.pop_back();
    }
    m_upstream.clear();
}

void PropertyBodyBase::_test_upstream(const Dependencies& dependencies)
{
    NOTF_ASSERT(_mutex().is_locked_by_this_thread());

    std::set<valid_ptr<PropertyBodyBase*>> unchecked;
    for (const PropertyReaderBase& reader : dependencies) {
        if (PropertyBodyBase* dependency = PropertyReaderBase::PropertyBodyAccess::property(reader).get()) {
            unchecked.insert(dependency);
        }
    }

    std::set<valid_ptr<PropertyBodyBase*>> checked;
    PropertyBodyBase* candidate;
    while (pop_one(unchecked, candidate)) {
        if (this == candidate) {
            notf_throw(PropertyGraph::no_dag_error,
                       "Failed to create property expression which would introduce a cyclic dependency");
        }
        checked.insert(candidate);
        for (const PropertyReaderBase& reader : candidate->m_upstream) {
            PropertyBodyBase* dependency = PropertyReaderBase::PropertyBodyAccess::property(reader).get();
            if (!checked.count(dependency)) {
                unchecked.emplace(dependency);
            }
        }
    }
}

void PropertyBodyBase::_set_upstream(Dependencies&& dependencies)
{
    NOTF_ASSERT(_mutex().is_locked_by_this_thread());

#ifdef NOTF_DEBUG
    _test_upstream(dependencies); // input should already have been tested at this point, but better safe than sorry
#endif

    // remove potential duplicates in the input
    m_upstream.clear();
    m_upstream.reserve(dependencies.size());
    for (const PropertyReaderBase& reader : dependencies) {
        if (std::find(m_upstream.cbegin(), m_upstream.cend(), reader) == m_upstream.cend()) {
            m_upstream.emplace_back(std::move(reader));
        }
    }

    // register with the upstream properties
    for (const PropertyReaderBase& reader : m_upstream) {
        PropertyBodyBase* dependency = PropertyReaderBase::PropertyBodyAccess::property(reader).get();
        dependency->_add_downstream(this);
    }
}

void PropertyBodyBase::_add_downstream(const valid_ptr<PropertyBodyBase*> affected)
{
    NOTF_ASSERT(_mutex().is_locked_by_this_thread());

#ifdef NOTF_DEBUG
    auto it = std::find(m_downstream.begin(), m_downstream.end(), affected);
    NOTF_ASSERT(it == m_downstream.end()); // there should not be a way to register the same property twice
#endif
    m_downstream.emplace_back(affected);
}

//====================================================================================================================//

PropertyHeadBase::~PropertyHeadBase() = default;

NOTF_CLOSE_NAMESPACE
