#pragma once

#include <forward_list>
#include <functional>
#include <map>
#include <memory>
#include <string>
#include <unordered_map>

#include "common/mutex.hpp"
#include "utils/type_name.hpp"

NOTF_OPEN_NAMESPACE

// ================================================================================================================== //

class ResourceManager {

    // types -------------------------------------------------------------------------------------------------------- //
private:
    /// Base class for all Resource types, used to store all subclasses into a single map.
    struct ResourceTypeBase {};

public:
    /// Exception thrown when an unknown Resource type is requested.
    NOTF_EXCEPTION_TYPE(type_error);

    /// Exception thrown when an unknown Resource is requested.
    NOTF_EXCEPTION_TYPE(resource_error);

    /// Exception thrown when a path of the ResourceManager could not be read.
    NOTF_EXCEPTION_TYPE(path_error);

    template<class T>
    class ResourceType : public ResourceTypeBase {
        friend class ResourceManager;

        /// Since the ResourceManager does not know how to actually load/create any type of resource, you have to supply
        /// a loading function to the Resource Type that takes the name of the resource file to load and returns a
        /// shared pointer to the created resource.
        /// If the shared pointer is empty, it is assumed that the load has failed and it will not be stored.
        using LoadFunction = std::function<std::shared_ptr<T>(const std::string&)>;

        /// Map storing all loaded resources by filename.
        using ResourceMap = std::map<const std::string, std::shared_ptr<T>>;

        // methods -------------------------------------------------------------------------------------------------- //
    private:
        NOTF_ALLOW_MAKE_SMART_FROM_PRIVATE;

        /// Constructor.
        /// @param manager  The manager managing this Resource type.
        /// @param loader   Load function for this Resource type.
        /// @param name     Name of the resource type for log messages (defaults to the demanged typename of T).
        ResourceType(ResourceManager& manager, LoadFunction loader, std::string name)
            : m_manager(manager), m_loader(std::move(loader)), m_name(std::move(name))
        {}

        /// Factory.
        /// @param mutex    Mutex protecting the Resource Manager.
        static std::unique_ptr<ResourceType> create(Mutex& mutex)
        {
            NOTF_MAKE_UNIQUE_FROM_PRIVATE(ResourceType, mutex);
        }

    public:
        /// Unique identifier for this resource type.
        size_t get_id() const { return m_id; }

        /// Name of the resource type for log messages.
        const std::string& get_name() const { return m_name; }

        /// Resource directory path relative to the ResourceManager's base path (can be empty).
        const std::string& get_path() const
        {
            NOTF_MUTEX_GUARD(m_manager.m_mutex);
            return m_path;
        }

        /// Sets a new directory path relative to the ResourceManager's base path.
        /// This method does not affect cached resources, only ones that are loaded in the future.
        /// @param path     New directory path. Can be empty.
        void set_path(std::string path)
        {
            if (!path.empty() && path.back() != '/') {
                path.append("/");
            }
            NOTF_MUTEX_GUARD(m_manager.m_mutex);
            m_path = std::move(path);
        }

        /// Number of inactive resources to retain in the cache.
        size_t get_cache_limit() const
        {
            NOTF_MUTEX_GUARD(m_manager.m_mutex);
            return m_cache_limit;
        }

        /// Updates the cache limit of this Resource type.
        /// @param cache_limit  Number of inactive resources to retain in the cache.
        void set_cache_limit(const size_t cache_limit)
        {
            NOTF_MUTEX_GUARD(m_manager.m_mutex);
            if (cache_limit < m_cache_limit) {
                _remove_inactive(cache_limit);
            }
            m_cache_limit = cache_limit;
        }

        /// Tests if a given resource is already cached.
        /// @param resource     Name of the resource file (not a path).
        bool is_cached(const std::string& resource)
        {
            NOTF_MUTEX_GUARD(m_manager.m_mutex);
            return m_resources.count(resource) != 0;
        }

        /// Returns a resource by its filename, either from the cache or by trying to load it.
        /// @param resource_name    Name of the resource file (not path).
        /// @throws resource_error  If the file could not be loaded.
        std::shared_ptr<T> get(const std::string& resource_name)
        {
            { // return a cached resource
                NOTF_MUTEX_GUARD(m_manager.m_mutex);
                auto it = m_resources.find(resource_name);
                if (it != m_resources.end()) {
                    return it->second;
                }
            }

            // the full path to the resource is: base_path + resource_type_path + resource_name
            std::string resource_path;
            resource_path.reserve(m_manager.m_base_path.size() + m_path.size() + resource_name.size());
            resource_path.append(m_manager.m_base_path);
            resource_path.append(m_path);
            resource_path.append(resource_name);

            // try to load the resource
            std::shared_ptr<T> resource;
            try {
                resource = m_loader(resource_path);
            }
            catch (const std::exception& exception) {
                NOTF_THROW(resource_error,
                           "Loader function failed while loading resource of type \"{}\" from \"{}\"\n"
                           "Error: {}",
                           type_name<T>(), resource_path, exception.what());
            }
            if (!resource) {
                NOTF_THROW(resource_error, "Failed to load resource \"{}\" of type \"{}\" from \"{}\"", resource_name,
                           type_name<T>(), resource_path);
            }

            { // cache the resource
                NOTF_MUTEX_GUARD(m_manager.m_mutex);

                // to avoid race conditions, see if another thread has loaded the same resource since we last checked
                auto it = m_resources.find(resource_name);
                if (it != m_resources.end()) {
                    return it->second;
                }

                // update the cache
                bool success = false;
                std::tie(it, success) = m_resources.emplace(resource_name, resource);
                NOTF_ASSERT(success);
                m_cache_list.emplace_front(std::move(it));
                _prune_inactive();
            }

            return resource;
        }

        /// Removes all inactive resources, ignoring this type's cache limit.
        void remove_all_inactive()
        {
            NOTF_MUTEX_GUARD(m_manager.m_mutex);
            _remove_inactive(0);
        }

    private:
        /// Removes inactive resources.
        /// @param cache_limit  How many of the most recently loaded inactive resources to retain.
        void _remove_inactive(const size_t cache_limit)
        {
            NOTF_ASSERT(m_manager.m_mutex.is_locked_by_this_thread());
            size_t inactive_count = 0;
            for (auto prev = m_cache_list.before_begin(), it = m_cache_list.begin(), end = m_cache_list.end();
                 it != end; prev = ++it) {
                while ((**it).unique() && ++inactive_count > cache_limit) {
                    it = m_cache_list.erase_after(prev);
                }
            }
        }

        /// Removes inactive resources with respect to this type's cache limit.
        void _prune_inactive() { _remove_inactive(m_cache_limit); }

        // fields --------------------------------------------------------------------------------------------------- //
    private:
        /// Unique identifier of this Resource type.
        const size_t m_id = typeid(T).hash_code();

        /// The manager managing this Resource type.
        ResourceManager& m_manager;

        /// Load function for this Resource type.
        const LoadFunction m_loader;

        /// Name of the resource type for log messages (defaults to the demanged name of T)
        const std::string m_name;

        /// Resource directory path relative to the ResourceManager's base path (can be empty).
        /// Always ends in a forward slash, if not empty.
        std::string m_path;

        /// Number of inactive resources to retain in the cache (defaults to 0).
        ///     0  = no caching
        ///     n = caching of n last loaded inactive
        /// Inactive means that the resource is held only by the ResourceManager.
        size_t m_cache_limit = 0;

        /// Resources by filename.
        ResourceMap m_resources;

        /// List of most recently loaded resources (newer resources are earlier in the list).
        std::forward_list<typename ResourceMap::Iterator> m_cache_list;
    };

    // methods ------------------------------------------------------------------------------------------------------ //
public:
    /// Constructor.
    /// @param base_path    Absolute path to the root directory of all managed resource files.
    /// @throws path_error  If `base_path` is not a directory.
    ResourceManager(std::string base_path) : m_base_path(std::move(base_path))
    {
        if (m_base_path.back() != '/') {
            m_base_path.append("/");
        }
    }

    /// Tests if a given type has an associated ResourceType.
    template<class U, typename T = strip_type_t<U>>
    bool has_type() const
    {
        static const size_t type_id = typeid(T).hash_code();

        NOTF_MUTEX_GUARD(m_mutex);
        return m_types.count(type_id) != 0;
    }

    /// Creates a new ResourceType.
    /// @param loader       Load function for this Resource type.
    /// @param name         Name of the resource type for log messages (defaults to the demanged typename of T).
    /// @throws type_error  If the ResourceType already exists.
    template<class U, typename T = strip_type_t<U>>
    void create_type(typename T::LoadFunction loader, std::string name = type_name<T>())
    {
        const size_t type_id = typeid(T).hash_code();

        NOTF_MUTEX_GUARD(m_mutex);
        if (has_type<T>()) {
            NOTF_THROW(type_error, "Resource type \"{}\" had already been registered with the ResourceManager", name);
        }
        bool success = false;
        std::tie(std::ignore, success)
            = m_types.emplace(type_id, std::make_unique<ResourceType<T>>(*this, std::move(loader), std::move(name)));
        NOTF_ASSERT(success);
    }

    /// Returns the ResourceType associated with the given type.
    /// @throws type_error  If the ResourceType does not exists.
    template<class U, typename T = strip_type_t<U>>
    const ResourceType<T>& get_type() const
    {
        static const size_t type_id = typeid(T).hash_code();

        NOTF_MUTEX_GUARD(m_mutex);
        auto it = m_types.find(type_id);
        if (it == m_types.end()) {
            NOTF_THROW(type_error,
                       "Unknown Resource type \"{0}\" requested from the ResourceManager.\n"
                       "Make sure to create it using ResourceManager::create_type<{0}>(...) first.",
                       type_name<T>());
        }
        return *(static_cast<ResourceType<T>*>(it->second.get()));
    }

private:
    /// Checks if a given string identifies a directory.
    /// @param path     String potentially identifying a directory.
    static bool _is_dir(const std::string& path);

    // fields ------------------------------------------------------------------------------------------------------- //
private:
    /// Absolute path to the root directory of all managed resource files.
    std::string m_base_path;

    /// Mutex protecting the ResourceManager and the ResourceTypes.
    mutable Mutex m_mutex;

    /// All ResourceTypes by their ID.
    mutable std::map<size_t, std::unique_ptr<ResourceTypeBase>> m_types;
};

NOTF_CLOSE_NAMESPACE
