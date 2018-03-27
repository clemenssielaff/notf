#include "app/property_graph.hpp"

#include "common/log.hpp"
#include "common/set.hpp"

NOTF_OPEN_NAMESPACE

//====================================================================================================================//

PropertyGraph::no_dag_error::~no_dag_error() = default;

//====================================================================================================================//

PropertyGraph::NodeBase::~NodeBase()
{
    _unregister_from_dependencies();
    for (NodeBase* id : m_affected) {
        if (risky_ptr<NodeBase> affected = PropertyGraph::instance().write_node(id)) {
            affected->_ground();
        }
    }
}

void PropertyGraph::NodeBase::_detect_cycles(const std::vector<NodeBase*>& dependencies)
{
    PropertyGraph& graph = PropertyGraph::instance();
    NOTF_ASSERT(graph.m_mutex.is_locked_by_this_thread());

    robin_set<NodeBase*> unchecked, checked;
    unchecked.reserve(dependencies.size());
    checked.reserve(dependencies.size());

    for (NodeBase* id : dependencies) {
        if (risky_ptr<NodeBase> dependency = graph.read_node(id)) {
            unchecked.insert(make_raw(dependency));
        }
    }

    NodeBase* const my_id = id();
    NodeBase* candidate;
    while (pop_one(unchecked, candidate)) {
        if (my_id == candidate) {
            notf_throw(no_dag_error, "Failed to create property expression which would introduce a cyclic "
                                     "dependency");
        }
        checked.insert(candidate);
        for (NodeBase* dependency : candidate->m_dependencies) {
            if (!checked.count(dependency)) {
                unchecked.emplace(dependency);
            }
        }
    }
}

//====================================================================================================================//

PropertyGraph::DeltaBase::~DeltaBase() = default;

PropertyGraph::ModificationDelta::~ModificationDelta() = default;

PropertyGraph::DeletionDelta::~DeletionDelta() = default;

//====================================================================================================================//

void PropertyGraph::freeze()
{
    const auto thread_id = std::this_thread::get_id();
    {
        std::lock_guard<Mutex> lock(m_mutex);
        if (thread_id == m_render_thread) {
            return;
        }
        if (thread_id != std::thread::id()) {
            notf_throw(thread_error, "Unexpected second reading thread of a PropertyGraph");
        }
        m_render_thread = thread_id;
    }
}

void PropertyGraph::unfreeze()
{
    const auto thread_id = std::this_thread::get_id();
    {
        std::lock_guard<Mutex> lock(m_mutex);
        if (thread_id != m_render_thread) {
            notf_throw(thread_error, "Only the render thread can unfreeze the PropertyGraph");
        }

        for (const auto& delta_it : m_delta) {
            auto node_it = m_nodes.find(delta_it.first);
            NOTF_ASSERT(node_it != m_nodes.end());
            DeltaBase& delta = *delta_it.second.get();

            if (delta.is_modification()) {
                // modification delta
                node_it.value()->resolve_delta(delta.node());
            }
            else {
                // deletion delta
                m_nodes.erase(node_it);
            }
        }
        m_delta.clear();
    }
}

NOTF_CLOSE_NAMESPACE
