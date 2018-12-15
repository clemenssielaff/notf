#include "notf/app/graph/graph.hpp"

#include "notf/app/graph/root_node.hpp"

#include "notf/common/mnemonic.hpp"

NOTF_OPEN_NAMESPACE
namespace detail {

// graph - node registry ============================================================================================ //

AnyNodeHandle Graph::NodeRegistry::get_node(Uuid uuid) const {
    NOTF_GUARD(std::lock_guard(m_mutex));
    if (auto iter = m_registry.find(uuid); iter != m_registry.end()) { return iter->second; }
    return {}; // no node found
}

void Graph::NodeRegistry::add(AnyNodePtr node) {
    // do not check whether this is the UI thread as we need this method during Application construction
    const Uuid& uuid = node->get_uuid();
    AnyNodeHandle handle = std::move(node);
    {
        NOTF_GUARD(std::lock_guard(m_mutex));
        const auto [iter, success] = m_registry.try_emplace(uuid, handle);
        if (NOTF_UNLIKELY(!success && iter->second != handle)) {
            // very unlikely, close to impossible without severe hacking and const-away casting
            NOTF_THROW(NotUniqueError, "A different Node with the UUID {} is already registered with the Graph",
                       uuid.to_string());
        }
    }
}

void Graph::NodeRegistry::remove(Uuid uuid) {
    NOTF_ASSERT(this_thread::is_the_ui_thread());
    NOTF_GUARD(std::lock_guard(m_mutex));
    if (auto iter = m_registry.find(uuid); iter != m_registry.end()) { m_registry.erase(iter); }
}

// the graph - node name registry =================================================================================== //

std::string Graph::NodeNameRegistry::set_name(AnyNodeHandle node, const std::string& name) {
    NOTF_ASSERT(this_thread::is_the_ui_thread());
    const Uuid& uuid = node.get_uuid(); // this might throw if the handle is expired, do it before locking the mutex
    {
        NOTF_GUARD(std::lock_guard(m_mutex));

        // if the node already exists under another name, we first have to unregister the old name
        if (const auto& name_view = m_uuid_to_name[uuid]; !name_view.empty()) { _remove_name(name_view); }

        std::string result;
        { // (re-)register the node under its proposed name, or a unique variant thereof
            size_t counter = 2;
            auto [iter, success] = m_name_to_node.try_emplace(name, node);
            while (!success) {
                std::tie(iter, success) = m_name_to_node.try_emplace(fmt::format("{}_{:0>2}", name, counter++), node);
            }
            m_uuid_to_name[uuid] = iter->first;
            result = iter->first;
        }
        return result;
    }
}

AnyNodeHandle Graph::NodeNameRegistry::get_node(const std::string& name) const {
    NOTF_GUARD(std::lock_guard(m_mutex));

    // find the handle...
    auto name_iter = m_name_to_node.find(name);
    if (name_iter == m_name_to_node.end()) { return {}; }

    // ...and return it
    AnyNodeHandle& node = name_iter->second;
    NOTF_ASSERT(!node.is_expired());
    return node;
}

std::string Graph::NodeNameRegistry::get_name(AnyNodeHandle node) {

    const Uuid& uuid = node.get_uuid(); // this might throw if the handle is expired, do it before locking the mutex
    {
        NOTF_GUARD(std::lock_guard(m_mutex));

        // find the name
        auto uuid_iter = m_uuid_to_name.find(uuid);
        if (uuid_iter != m_uuid_to_name.end()) { return std::string(uuid_iter->second); }

        // if the name doesn't exist, make one up
        return set_name(std::move(node), number_to_mnemonic(hash(uuid), /*max_syllables=*/4));
    }
}

void Graph::NodeNameRegistry::remove_node(const Uuid uuid) {
    NOTF_ASSERT(this_thread::is_the_ui_thread());
    NOTF_GUARD(std::lock_guard(m_mutex));
    if (auto uuid_iter = m_uuid_to_name.find(uuid); uuid_iter != m_uuid_to_name.end()) {
        _remove_name(uuid_iter->second);
        m_uuid_to_name.erase(uuid_iter);
    }
}

void Graph::NodeNameRegistry::_remove_name(std::string_view name_view) {
    NOTF_ASSERT(m_mutex.is_locked_by_this_thread());
    static std::string name; // copy the value of the string_view into a reusable static string
    name = name_view;
    auto iter = m_name_to_node.find(name);
    if (iter != m_name_to_node.end()) { m_name_to_node.erase(iter); }
}

// graph ============================================================================================================ //

Graph::Graph() {
    // create the new root node ...
    m_root_node = std::make_shared<RootNode>();
    RootNode::AccessFor<Graph>::finalize(*m_root_node);

    // ... and register it
    AnyNodePtr node = std::static_pointer_cast<AnyNode>(m_root_node);
    m_node_registry.add(node);
    m_dirty_nodes.emplace(std::move(node));
}

Graph::~Graph() {
    m_root_node.reset(); // erase all nodes by erasing the root
}

RootNodeHandle Graph::get_root_node() { return m_root_node; }

bool Graph::synchronize() {
    NOTF_ASSERT(this_thread::is_the_ui_thread());

    if (m_dirty_nodes.empty()) {
        return false; // nothing changed
    }

    for (auto& handle : m_dirty_nodes) {
        if (AnyNodePtr node = AnyNodeHandle::AccessFor<Graph>::get_node_ptr(handle)) {
            AnyNode::AccessFor<Graph>::clear_modified_data(*node);
        }
    }
    m_dirty_nodes.clear();
    return true; // dirty nodes cleared their modified data
}

} // namespace detail

NOTF_CLOSE_NAMESPACE
