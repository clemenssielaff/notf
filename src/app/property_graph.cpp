#include "app/property_graph.hpp"

#include <algorithm>
#include <map>
#include <set>

#include "app/application.hpp"
#include "app/event_manager.hpp"
#include "app/io/property_event.hpp"
#include "app/node.hpp"
#include "app/scene_graph.hpp"
#include "app/window.hpp"
#include "common/set.hpp"

NOTF_OPEN_NAMESPACE

// ================================================================================================================== //

PropertyGraph::no_dag_error::~no_dag_error() = default;

void PropertyGraph::fire_event(PropertyUpdateList&& effects)
{
    // sort effects by Window containing the affected NodeProperty
    std::map<valid_ptr<const Window*>, PropertyUpdateList> updates_by_window;
    while (!effects.empty()) {
        PropertyUpdatePtr update = std::move(effects.back());
        effects.pop_back();

        if (risky_ptr<PropertyHead*> property_head = update->property->head()) {
            if (risky_ptr<Node*> node = property_head->node()) {
                WindowPtr window;
                if (risky_ptr<WindowPtr> risky = node->graph().getWindow()) {
                    window = std::move(risky);
                }
                else {
                    continue;
                }
                if (updates_by_window.count(window.get()) == 0) {
                    PropertyUpdateList window_updates;
                    window_updates.reserve(effects.size() + 1); // assume all updates are for the same window
                    window_updates.emplace_back(std::move(update));
                    updates_by_window[window.get()] = std::move(window_updates);
                }
                else {
                    updates_by_window[window.get()].emplace_back(std::move(update));
                }
            }
        }
    }

    // fire events
    while (!updates_by_window.empty()) {
        auto it = updates_by_window.begin();

        Application::instance().event_manager().handle(
            std::make_unique<PropertyEvent>(it->first, std::move(it->second)));
        updates_by_window.erase(it);
    }
}

RecursiveMutex PropertyGraph::s_mutex = {};

#ifdef NOTF_TEST
std::atomic_size_t PropertyGraph::s_body_count{0};
#endif

// ================================================================================================================== //

PropertyUpdate::~PropertyUpdate() = default;

// ================================================================================================================== //

PropertyBody::~PropertyBody()
{
    { // properties that are getting deleted should not have any downstream left, but to be sure, ground here
        NOTF_MUTEX_GUARD(_mutex());
        PropertyBody::_ground();
    }

#ifdef NOTF_TEST
    --PropertyGraph::Access<PropertyBody>::property_count();
#endif
}

void PropertyBody::_ground()
{
    NOTF_ASSERT(_mutex().is_locked_by_this_thread());

    for (const PropertyReader& reader : m_upstream) {
        PropertyBody* dependency = PropertyReader::Access<PropertyBody>::property(reader).get();
        auto it = std::find(dependency->m_downstream.begin(), dependency->m_downstream.end(), this);
        NOTF_ASSERT(it != dependency->m_downstream.end());
        *it = std::move(dependency->m_downstream.back());
        dependency->m_downstream.pop_back();
    }
    m_upstream.clear();
}

void PropertyBody::_test_upstream(const Dependencies& dependencies)
{
    NOTF_ASSERT(_mutex().is_locked_by_this_thread());

    std::set<valid_ptr<PropertyBody*>> unchecked;
    for (const PropertyReader& reader : dependencies) {
        if (PropertyBody* dependency = PropertyReader::Access<PropertyBody>::property(reader).get()) {
            unchecked.insert(dependency);
        }
    }

    std::set<valid_ptr<PropertyBody*>> checked;
    PropertyBody* candidate;
    while (pop_one(unchecked, candidate)) {
        if (this == candidate) {
            notf_throw(PropertyGraph::no_dag_error,
                       "Failed to create property expression which would introduce a cyclic dependency");
        }
        checked.insert(candidate);
        for (const PropertyReader& reader : candidate->m_upstream) {
            PropertyBody* dependency = PropertyReader::Access<PropertyBody>::property(reader).get();
            if (!checked.count(dependency)) {
                unchecked.emplace(dependency);
            }
        }
    }
}

void PropertyBody::_set_upstream(Dependencies&& dependencies)
{
    NOTF_ASSERT(_mutex().is_locked_by_this_thread());

#ifdef NOTF_DEBUG
    _test_upstream(dependencies); // input should already have been tested at this point, but better safe than sorry
#endif

    // remove potential duplicates in the input
    m_upstream.clear();
    m_upstream.reserve(dependencies.size());
    for (const PropertyReader& reader : dependencies) {
        if (std::find(m_upstream.cbegin(), m_upstream.cend(), reader) == m_upstream.cend()) {
            m_upstream.emplace_back(std::move(reader));
        }
    }

    // register with the upstream properties
    for (const PropertyReader& reader : m_upstream) {
        PropertyBody* dependency = PropertyReader::Access<PropertyBody>::property(reader).get();
        dependency->_add_downstream(this);
    }
}

void PropertyBody::_add_downstream(const valid_ptr<PropertyBody*> affected)
{
    NOTF_ASSERT(_mutex().is_locked_by_this_thread());

#ifdef NOTF_DEBUG
    auto it = std::find(m_downstream.begin(), m_downstream.end(), affected);
    NOTF_ASSERT(it == m_downstream.end()); // there should not be a way to register the same property twice
#endif
    m_downstream.emplace_back(affected);
}

void PropertyBody::_remove_head()
{
    NOTF_MUTEX_GUARD(_mutex());
    m_head = nullptr;
}

// ================================================================================================================== //

PropertyHead::~PropertyHead()
{
    if (m_body) {
        PropertyBody::Access<PropertyHead>::remove_head(*m_body);
    }
}

NOTF_CLOSE_NAMESPACE
