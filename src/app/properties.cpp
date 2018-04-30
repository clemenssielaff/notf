#include "app/properties.hpp"

#include <algorithm>

#include "app/application.hpp"
#include "app/event_manager.hpp"
#include "app/io/property_event.hpp"
#include "app/scene.hpp"
#include "common/set.hpp"

NOTF_OPEN_NAMESPACE

//====================================================================================================================//

PropertyGraph::no_dag::~no_dag() = default;

PropertyGraph::no_graph::~no_graph() = default;

PropertyGraph::Update::~Update() = default;

//====================================================================================================================//

PropertyGraph::PropertyBody::~PropertyBody() = default;

void PropertyGraph::PropertyBody::prepare_removal(const PropertyGraph& graph)
{
    NOTF_ASSERT(graph.m_mutex.is_locked_by_this_thread());
    _ground(graph);
    for (valid_ptr<PropertyBody*> affected : m_downstream) {
        affected->_ground(graph);
    }
}

void PropertyGraph::PropertyBody::_ground(const PropertyGraph& graph)
{
    NOTF_ASSERT(graph.m_mutex.is_locked_by_this_thread());
    for (valid_ptr<PropertyBody*> dependency : m_upstream) {
        auto it = std::find(dependency->m_downstream.begin(), dependency->m_downstream.end(), this);
        NOTF_ASSERT(it != dependency->m_downstream.end());

        *it = std::move(dependency->m_downstream.back());
        dependency->m_downstream.pop_back();
    }
    m_upstream.clear();
}

bool PropertyGraph::PropertyBody::_validate_upstream(const PropertyGraph& graph, const Connected& dependencies) const
{
    NOTF_ASSERT(graph.m_mutex.is_locked_by_this_thread());

    // fill the unordered set and
    std::set<valid_ptr<PropertyBody*>> unchecked;
    for (valid_ptr<PropertyBody*> dependency : dependencies) {
        // check if all properties are still alive
        if (!graph.m_properties.count(dependency)) {
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

//====================================================================================================================//

PropertyGraph::PropertyHead::~PropertyHead()
{
    if (auto property_graph = _graph()) {
        property_graph->_delete_property(m_body);
    }
}

void PropertyGraph::PropertyHead::_fire_event(UpdateSet&& affected)
{
    if (auto property_graph = _graph()) {
        valid_ptr<Window*> window = &(property_graph->m_scene_graph.window());
        Application::instance().event_manager().handle(std::make_unique<PropertyEvent>(window, std::move(affected)));
    }
}

//====================================================================================================================//

void PropertyGraph::Batch::execute()
{
    PropertyGraphPtr property_graph = m_graph.lock();
    if (!property_graph) {
        notf_throw(no_graph, "PropertyGraph has been deleted");
    }

    PropertyGraph::UpdateSet affected;
    {
        std::unique_lock<Mutex> lock(property_graph->m_mutex);

        // verify that all updates will succeed first
        for (auto& update : m_updates) {
            update->property->_validate_update(update.get());
        }

        // apply the updates
        for (auto& update : m_updates) {
            update->property->_apply_update(*property_graph, update.get(), affected);
        }
    }

    // fire off the combined event
    valid_ptr<Window*> window = &(property_graph->m_scene_graph.window());
    Application::instance().event_manager().handle(std::make_unique<PropertyEvent>(window, std::move(affected)));

    // reset in case the user wants to reuse the batch object
    m_updates.clear();
}

//====================================================================================================================//

bool PropertyGraph::is_frozen() const
{
    return m_scene_graph.is_frozen();
}

bool PropertyGraph::is_frozen_by(const std::thread::id thread_id) const
{
    return m_scene_graph.is_frozen_by(std::move(thread_id));
}

//====================================================================================================================//

namespace detail {

PropertyHandlerBase::~PropertyHandlerBase() = default;

} // namespace detail

NOTF_CLOSE_NAMESPACE
