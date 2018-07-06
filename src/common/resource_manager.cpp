#include "common/resource_manager.hpp"

#if __has_include("filesystem")
#include <filesystem>
namespace filesystem = std::filesystem;

#elif __has_include("experimental/filesystem")
#include <experimental/filesystem>
namespace filesystem = std::experimental::filesystem;

#else
#define NOTF_NO_FILESYSTEM
#endif

NOTF_OPEN_NAMESPACE

// ================================================================================================================== //

ResourceManager::type_error::~type_error() = default;

ResourceManager::resource_error::~resource_error() = default;

ResourceManager::path_error::~path_error() = default;

// ================================================================================================================== //

bool ResourceManager::_is_dir(const std::string& path)
{
#ifdef NOTF_NO_FILESYSTEM
    return true;
#endif
    return filesystem::is_directory(path);
}

NOTF_CLOSE_NAMESPACE
