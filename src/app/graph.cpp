#include "notf/app/graph.hpp"

#include "notf/common/mnemonic.hpp"

#include "notf/app/root_node.hpp"

NOTF_OPEN_NAMESPACE

// the graph - node registry ======================================================================================== //

NodeHandle TheGraph::NodeRegistry::get_node(Uuid uuid) const
{
    NOTF_GUARD(std::lock_guard(m_mutex));
    if (auto iter = m_registry.find(uuid); iter != m_registry.end()) { return iter->second; }
    return {}; // no node found
}

void TheGraph::NodeRegistry::add(NodeHandle node)
{
    const Uuid& uuid = node.get_uuid();
    {
        NOTF_GUARD(std::lock_guard(m_mutex));
        const auto [iter, success] = m_registry.try_emplace(uuid, node);
        if (!success && iter->second != node) {
            NOTF_THROW(NotUniqueError, "A different Node with the UUID {} is already registered with the Graph",
                       uuid.to_string());
        }
    }
}

void TheGraph::NodeRegistry::remove(Uuid uuid)
{
    NOTF_GUARD(std::lock_guard(m_mutex));
    if (auto iter = m_registry.find(uuid); iter != m_registry.end()) { m_registry.erase(iter); }
}

// the graph - node name registry =================================================================================== //

std::string_view TheGraph::NodeNameRegistry::get_name(NodeHandle node) const
{
    const Uuid& uuid = node.get_uuid(); // this might throw if the handle is expired, do it before locking the mutex
    {
        NOTF_GUARD(std::lock_guard(m_mutex));

        // get or create new entry
        auto& name_view = m_uuid_to_name[uuid];
        if (!name_view.empty()) { return name_view; }

        std::string name;
        { // generate and a new, random, and unique name
            const std::string base_name = number_to_mnemonic(hash(node.get_uuid()), /*max_syllables=*/4);
            size_t counter = 2;
            name = base_name;
            while (m_name_to_node.count(name) > 0) {
                name = fmt::format("{}_{:0>2}", name, counter++);
            }
        }

        // store the node handle under its new name
        NOTF_ASSERT(m_name_to_node.count(name) == 0);
        auto [iter, success] = m_name_to_node.emplace(std::move(name), std::make_pair(uuid, node));
        NOTF_ASSERT(success);
        name_view = iter->first;

        return name_view;
    }
}

std::string_view TheGraph::NodeNameRegistry::set_name(NodeHandle node, const std::string& name)
{
    const Uuid& uuid = node.get_uuid(); // this might throw if the handle is expired, do it before locking the mutex
    {
        NOTF_GUARD(std::lock_guard(m_mutex));

        // get or create new entry
        auto& name_view = m_uuid_to_name[uuid];

        // if the node already exists under another name, we first have to unregister the old name
        if (!name_view.empty()) {
            _remove_name(name_view);

            auto iter = m_uuid_to_name.find(uuid);
            NOTF_ASSERT(iter != m_uuid_to_name.end());
            m_uuid_to_name.erase(iter);
        }

        { // (re-)register the node under its proposed name, or a unique variant thereof
            size_t counter = 2;
            auto [iter, success] = m_name_to_node.try_emplace(name, std::make_pair(uuid, node));
            while (!success) {
                std::tie(iter, success) = m_name_to_node.try_emplace(fmt::format("{}_{:0>2}", name, counter++), //
                                                                     std::make_pair(uuid, node));
                // if the name is already taken, but the handle expired, just replace it
                if (!success && iter->second.second.is_expired()) {
                    iter->second = std::make_pair(uuid, node);
                    success = true;
                }
            }
            name_view = iter->first;
        }

        return name_view;
    }
}

NodeHandle TheGraph::NodeNameRegistry::get_node(const std::string& name) const
{
    Uuid uuid;
    {
        NOTF_GUARD(std::lock_guard(m_mutex));

        // find the handle
        auto name_iter = m_name_to_node.find(name);
        if (name_iter == m_name_to_node.end()) { return {}; }

        // if the handle is valid, return it
        NodeHandle& node = name_iter->second.second;
        if (!node.is_expired()) { return node; }

        // if the handle has expired, make sure it is erased from all the maps
        uuid = name_iter->second.first;
        m_name_to_node.erase(name_iter);
        if (auto uuid_iter = m_uuid_to_name.find(uuid); uuid_iter != m_uuid_to_name.end()) {
            m_uuid_to_name.erase(uuid_iter);
        }
    }

    // also remove the node from the NodeRegistry (without holding another mutex to avoid deadlock
    TheGraph()._get().m_node_registry.remove(uuid);

    return {};
}

void TheGraph::NodeNameRegistry::remove_node(const Uuid uuid)
{
    NOTF_GUARD(std::lock_guard(m_mutex));
    if (auto uuid_iter = m_uuid_to_name.find(uuid); uuid_iter != m_uuid_to_name.end()) {
        _remove_name(uuid_iter->second);
        m_uuid_to_name.erase(uuid_iter);
    }
}

void TheGraph::NodeNameRegistry::_remove_name(std::string_view name_view)
{
    NOTF_ASSERT(m_mutex.is_locked_by_this_thread());
    static std::string name; // copy the value of the string_view into a reusable static string
    name = name_view;
    auto iter = m_name_to_node.find(name);
    if (iter != m_name_to_node.end()) { m_name_to_node.erase(iter); }
}

// the graph ======================================================================================================== //

TheGraph::TheGraph()
{
    // the default root node has no properties
    initialize<CompileTimeRootNode<EmptyNodePolicy>>();
}

TheGraph::~TheGraph()
{
    // erase all nodes that you own
    m_modified_root_node.reset();
    m_root_node.reset();
}

NodeHandle TheGraph::get_root_node(const std::thread::id thread_id)
{
    TheGraph& graph = _get();

    if (is_frozen_by(thread_id)) {
        return std::dynamic_pointer_cast<Node>(graph.m_root_node); // the renderer always sees the unmodified root node
    }

    // if there exist a modified root node, return that one instead
    if (graph.m_modified_root_node != nullptr) {
        NOTF_ASSERT(TheGraph::is_frozen());
        return std::dynamic_pointer_cast<Node>(graph.m_modified_root_node);
    }

    // either the root node has not been modified, or the Graph is not frozen
    return std::dynamic_pointer_cast<Node>(graph.m_root_node);
}

void TheGraph::_initialize_untyped(AnyRootNodePtr&& root_node)
{
    NOTF_GUARD(std::lock_guard(m_graph_mutex));

    if (is_frozen()) {
        NOTF_ASSERT(m_root_node);

        // the render thread must never re-initialize the graph
        NOTF_ASSERT(!is_frozen_by(std::this_thread::get_id()));

        // either create a new modified root node or replace a previous modification
        m_modified_root_node = root_node;
    }

    // if the Graph is not frozen, replace the current root node
    else {
        m_root_node = root_node;
    }

    root_node->finalize();
}

NOTF_CLOSE_NAMESPACE
