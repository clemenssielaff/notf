#include "common/resource_manager.hpp"

NOTF_OPEN_NAMESPACE

// ================================================================================================================== //

ResourceManager::type_error::~type_error() = default;

ResourceManager::resource_error::~resource_error() = default;

ResourceManager::path_error::~path_error() = default;

// ================================================================================================================== //

ResourceManager::ResourceTypeBase::~ResourceTypeBase() = default;

void ResourceManager::cleanup()
{
    NOTF_MUTEX_GUARD(m_mutex);
    for (auto& type : m_types) {
        type.second->_cleanup();
    }
}

void ResourceManager::clear()
{
    NOTF_MUTEX_GUARD(m_mutex);
    for (auto& type : m_types) {
        type.second->_clear();
    }
}

std::string ResourceManager::_ensure_is_dir(const std::string& path)
{
    // TODO: use std::filesystem or equivalent for the resource manager
    //       also for `_ensure_is_subdir` - the current implementation does not much at all
    if (path.back() == '/') {
        return path;
    }

    std::string result;
    result.reserve(path.size() + 1);
    result.append(path);
    result.append("/");
    return result;
}

std::string ResourceManager::_ensure_is_subdir(const std::string& path) { return _ensure_is_dir(path); }

NOTF_CLOSE_NAMESPACE
