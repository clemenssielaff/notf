#pragma once

#include <forward_list>
#include <map>
#include <memory>
#include <string>
#include <unordered_map>

#include "notf/meta/smart_ptr.hpp"
#include "notf/meta/typename.hpp"

#include "notf/common/mutex.hpp"

NOTF_OPEN_NAMESPACE

// resource handle ================================================================================================== //

template<class T>
class ResourceHandle {

    // methods --------------------------------------------------------------------------------- //
public:
    /// Empty default constructor.
    ResourceHandle() = default;

    /// Constructor.
    /// @param resource     Handled resource.
    explicit ResourceHandle(std::shared_ptr<T> resource) : m_resource(std::move(resource)) {}

    /// Copy Constructor.
    /// @param other    ResourceHandle to copy.
    ResourceHandle(const ResourceHandle& other) : m_resource(other.m_resource) {}

    /// Move Constructor.
    /// @param other    ResourceHandle to move from.
    ResourceHandle(ResourceHandle&& other) : m_resource(std::move(other.m_resource)) { other.m_resource.reset(); }

    /// Copy operator.
    /// @param other    ResourceHandle to copy.
    ResourceHandle& operator=(const ResourceHandle& other) { m_resource = other.m_resource; }

    /// Move operator.
    /// @param other    ResourceHandle to copy.
    ResourceHandle& operator=(ResourceHandle&& other) {
        m_resource = std::move(other.m_resource);
        other.m_resource.reset();
        return *this;
    }

    /// Comparison operator.
    /// @param other    ResourceHandle to compare with.
    bool operator==(const ResourceHandle& other) const { return m_resource == other.m_resource; }

    /// The managed resource.
    /// @throws resource_error  If the handle is not valid.
    operator T*() {
        if (!is_valid()) {
            NOTF_THROW(resource_error, "Cannot acces invalid Handle for resource of type \"{}\"", type_name<T>());
        }
        return m_resource.get();
    }
    T* operator->() { return operator T*(); }

    /// Returns the shared pointer contained in this Resource handle.
    std::shared_ptr<T> get_shared() { return m_resource; }

    /// Tests if the ResourceHandle is valid.
    bool is_valid() const { return m_resource != nullptr; }
    explicit operator bool() const { return is_valid(); }
    bool operator!() const { return !is_valid(); }

    // TODO: methods for manual or even automatic reload for resources that have changed on disk
    //       you could do that by storing a "version" holder in the type, instead of shared_ptrs to the resource
    //       and each handle would keep a shared_ptr to the "version" and an ID where 0 (or max) means "always
    //       the latest". Every time the resource it requested, you would copy a shared_ptr (which should be atomic,
    //       right?) to the actual resource and use that, in case the version holder changes while you work on the
    //       resource.

    // fields ---------------------------------------------------------------------------------- //
private:
    /// Handled resource.
    std::shared_ptr<T> m_resource;
};

// resource manager ================================================================================================= //

// TODO: ScopedInstance for ResourceManager
class ResourceManager {

    // types ----------------------------------------------------------------------------------- //
private:
    /// Base class for all Resource types, used to store all subclasses into a single map.
    class ResourceTypeBase {
        friend class ResourceManager;

        // methods ----------------------------------------------------------------------------- //
    public:
        /// Destructor.
        virtual ~ResourceTypeBase();

    private:
        /// Removes inactive resources.
        /// @param cache_limit  How many of the most recently loaded inactive resources to retain.
        virtual void _remove_inactive(const size_t cache_limit) = 0;

        /// Removes all resources, inactive or not.
        virtual void _clear() = 0;

        /// Removes inactive resources as determined by the cache limit.
        void _cleanup() { _remove_inactive(m_cache_limit); }

        // fields ------------------------------------------------------------------------------ //
    protected:
        /// Number of inactive resources to retain in the cache (defaults to 0).
        ///     0  = no caching
        ///     n = caching of n last loaded inactive
        /// Inactive means that the resource is held only by the ResourceManager.
        size_t m_cache_limit = 0;
    };

public:
    template<class T>
    class ResourceType : public ResourceTypeBase {
        friend class ResourceManager;

        /// Map storing all loaded resources by filename.
        using ResourceMap = std::map<const std::string, std::shared_ptr<T>>;

        // methods ----------------------------------------------------------------------------- //
    private:
        NOTF_CREATE_SMART_FACTORIES(ResourceType);

        /// Constructor.
        /// @param manager  The manager managing this Resource type.
        ResourceType(ResourceManager& manager) : ResourceTypeBase(), m_manager(manager) {}

        /// Factory.
        /// @param manager  The manager managing this Resource type.
        static std::unique_ptr<ResourceType<T>> create(ResourceManager& manager) { return _create_unique(manager); }

    public:
        NOTF_NO_COPY_OR_ASSIGN(ResourceType);

        /// Unique identifier for this resource type.
        size_t get_id() const { return m_id; }

        /// Name of the resource type for log messages.
        const std::string& get_name() const { return m_name; }

        /// Resource directory path relative to the ResourceManager's base path (can be empty).
        const std::string& get_path() const {
            NOTF_GUARD(std::lock_guard(m_manager.m_mutex));
            return m_path;
        }

        /// Sets a new directory path relative to the ResourceManager's base path.
        /// This method does not affect cached resources, only ones that are loaded in the future.
        /// @param path     New directory path. Can be empty.
        void set_path(const std::string& path) {
            std::string directory_path = ResourceManager::_ensure_is_dir(path);
            {
                NOTF_GUARD(std::lock_guard(m_manager.m_mutex));
                m_path.reserve(m_manager.m_base_path.size() + directory_path.size());
                m_path = m_manager.m_base_path;
                m_path.append(std::move(directory_path));
            }
        }

        /// Number of inactive resources to retain in the cache.
        size_t get_cache_limit() const {
            NOTF_GUARD(std::lock_guard(m_manager.m_mutex));
            return m_cache_limit;
        }

        /// Updates the cache limit of this Resource type.
        /// @param cache_limit  Number of inactive resources to retain in the cache.
        void set_cache_limit(const size_t cache_limit) {
            NOTF_GUARD(std::lock_guard(m_manager.m_mutex));
            if (cache_limit < m_cache_limit) { _remove_inactive(cache_limit); }
            m_cache_limit = cache_limit;
        }

        /// Returns a cached resource by its filename.
        /// @param name             Name of the resource.
        /// @throws resource_error  If there is no resource by the name.
        ResourceHandle<T> get(const std::string& name) const {
            NOTF_GUARD(std::lock_guard(m_manager.m_mutex));
            auto it = m_resources.find(name);
            if (it != m_resources.end()) { return ResourceHandle<T>(it->second); }
            return {};
        }

        ResourceHandle<T> set(const std::string& name, std::shared_ptr<T> resource) {
            NOTF_GUARD(std::lock_guard(m_manager.m_mutex));

            auto it = m_resources.find(name);

            if (it == m_resources.end()) {
                // create a new resource entry
                bool success = false;
                std::tie(it, success) = m_resources.emplace(name, std::move(resource));
                NOTF_ASSERT(success);

                m_cache.emplace_front(it);
                _remove_inactive(m_cache_limit);
                return ResourceHandle<T>(it->second);
            }

            else {
                // remove the existing entry from the list of most recently loaded
                bool success = false;
                for (auto prev = m_cache.before_begin(), cache_it = m_cache.begin(), end = m_cache.end();
                     cache_it != end; prev = ++cache_it) {
                    if (*cache_it == it) {
                        m_cache.erase_after(prev);
                        success = true;
                        break;
                    }
                }
                NOTF_ASSERT(success);

                // update the existing entry
                it->second = std::move(resource);
                m_cache.emplace_front(it);
                return ResourceHandle<T>(it->second);
            }
        }

        /// Removes all inactive resources, ignoring this type's cache limit.
        void remove_all_inactive() {
            NOTF_GUARD(std::lock_guard(m_manager.m_mutex));
            _remove_inactive(0);
        }

    private:
        /// Removes inactive resources.
        /// @param cache_limit  How many of the most recently loaded inactive resources to retain.
        void _remove_inactive(const size_t cache_limit) final {
            NOTF_ASSERT(m_manager.m_mutex.is_locked_by_this_thread());
            size_t inactive_count = 0;
            for (auto prev = m_cache.before_begin(), it = m_cache.begin(), end = m_cache.end();; prev = ++it) {
                while (it != end && (**it).second.unique() && ++inactive_count > cache_limit) {
                    m_resources.erase(*it);
                    it = m_cache.erase_after(prev);
                }
                if (it == end) { break; }
            }
        }

        /// Removes all resources, inactive or not.
        void _clear() final {
            NOTF_ASSERT(m_manager.m_mutex.is_locked_by_this_thread());
            m_cache.clear();
            m_resources.clear();
        }

        // fields ------------------------------------------------------------------------------ //
    private:
        /// Unique identifier of this Resource type.
        const size_t m_id = typeid(T).hash_code();

        /// The manager managing this Resource type.
        ResourceManager& m_manager;

        /// Name of the resource type for log messages.
        const std::string m_name = type_name<T>();

        /// Resource directory path relative to the ResourceManager's base path (can be empty).
        /// Always ends in a forward slash, if not empty.
        std::string m_path;

        /// Resources by filename.
        ResourceMap m_resources;

        /// List of most recently loaded resources (newer resources are earlier in the list).
        std::forward_list<typename ResourceMap::iterator> m_cache;
    };

    // methods --------------------------------------------------------------------------------- //
private:
    /// Constructor.
    ResourceManager() = default;

public:
    /// Returns the global ResourceManager.
    static ResourceManager& get_instance() {
        static ResourceManager instance;
        return instance;
    }

    /// Returns the ResourceType associated with the given type.
    /// @throws type_error  If the ResourceType does not exists.
    template<class U, typename T = std::decay_t<U>>
    ResourceType<T>& get_type() {
        static const size_t type_id = typeid(T).hash_code();

        NOTF_GUARD(std::lock_guard(m_mutex));
        decltype(m_types)::iterator it = m_types.find(type_id);
        if (it == m_types.end()) {
            bool success = false;
            std::tie(it, success) = m_types.emplace(std::make_pair(type_id, ResourceType<T>::create(*this)));
            NOTF_ASSERT(success);
        }
        return *static_cast<ResourceType<T>*>(it->second.get());
    }

    /// Absolute path to the root directory of all managed resource files.
    const std::string& get_base_path() const { return m_base_path; }

    /// Sets a new base path of the Resource Manager.
    /// @param base_path    Absolute path to the root directory of all managed resource files.
    /// @throws path_error  If `base_path` is not a directory.
    void set_base_path(const std::string& base_path) { m_base_path = _ensure_is_dir(base_path); }

    /// Deletes all inactive resources.
    void cleanup();

    /// Releases ownership of all managed resources.
    /// If a resource is not currently in use by another object owning a shared pointer to it, it is deleted.
    void clear();

private:
    /// Checks if a given string identifies a directory.
    /// @param path         String potentially identifying a directory.
    /// @returns            String identifying an absolute, normalized directory.
    /// @throws path_error  If `path` does not identify a valid directory.
    static std::string _ensure_is_dir(const std::string& path);

    /// Checks if a given string identifies a subdirectory of this ResourceManager's base directory.
    /// @param path         String potentially identifying a directory.
    /// @returns            String identifying a normalized directory relative to this Manager's base directory.
    /// @throws path_error  If `path` does not identify a valid subdirectory.
    static std::string _ensure_is_subdir(const std::string& path);

    // fields ---------------------------------------------------------------------------------- //
private:
    /// Absolute path to the root directory of all managed resource files.
    std::string m_base_path;

    /// Mutex protecting the ResourceManager and the ResourceTypes.
    mutable Mutex m_mutex;

    /// All ResourceTypes by their ID.
    mutable std::map<size_t, std::unique_ptr<ResourceTypeBase>> m_types;
};

NOTF_CLOSE_NAMESPACE
