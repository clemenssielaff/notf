#pragma once

#include <algorithm>
#include <memory>
#include <vector>

#include "graphics/forwards.hpp"

NOTF_OPEN_NAMESPACE

// ================================================================================================================== //

template<typename INSTANCE_DATA>
class PrefabInstance {

    template<typename>
    friend class PrefabType;

    // types -------------------------------------------------------------------------------------------------------- //
public:
    using InstanceData = INSTANCE_DATA;

    // methods ------------------------------------------------------------------------------------------------------ //
private:
    NOTF_ALLOW_MAKE_SMART_FROM_PRIVATE;

    /// Constructor.
    /// @param prefab_type   Owning reference to this instance's prefab type.
    PrefabInstance(std::shared_ptr<PrefabType<InstanceData>> prefab_type) : m_prefab(std::move(prefab_type)) {}

    /// Factory.
    static std::shared_ptr<PrefabInstance<InstanceData>> create(std::shared_ptr<PrefabType<InstanceData>> prefab)
    {
        return NOTF_MAKE_SHARED_FROM_PRIVATE(PrefabInstance<InstanceData>, std::move(prefab));
    }

public:
    /// Read-write access to the prefab's instance data.
    InstanceData& get_data() { return m_data; }

    // fields ------------------------------------------------------------------------------------------------------- //
private:
    /// Offset into the library's index buffer, where this prefab starts.
    std::shared_ptr<PrefabType<InstanceData>> m_prefab;

    /// Per-instance data for this prefab.
    InstanceData m_data;
};

// ================================================================================================================== //

/// A prefab type is an entry into a prefab group that defines how an object is rendered.
/// In order to draw a prefab type on the screen, you need to create a prefab instance of the type.
template<typename INSTANCE_DATA>
class PrefabType : public std::enable_shared_from_this<PrefabType<INSTANCE_DATA>> {

    template<typename>
    friend class PrefabFactory;

    // types -------------------------------------------------------------------------------------------------------- //
public:
    using InstanceData = INSTANCE_DATA;

    // methods ------------------------------------------------------------------------------------------------------ //
private:
    NOTF_ALLOW_MAKE_SMART_FROM_PRIVATE;

    /// Constructor.
    /// @param name      Name of this prefab type.
    /// @param offset    Offset into the group's index buffer, where this prefab starts.
    /// @param size      Number of indices that make up this prefab in the group.
    PrefabType(const std::string name, const size_t offset, const size_t size)
        : m_name(name), m_offset(offset), m_size(static_cast<GLsizei>(size))
    {}

    /// Method called by the factory for creating a new prefab type.
    static std::shared_ptr<PrefabType<InstanceData>>
    create(const std::string name, const size_t offset, const size_t size)
    {
        return NOTF_MAKE_SHARED_FROM_PRIVATE(PrefabType<InstanceData>, std::move(name), offset, size);
    }

public:
    /// Factory method.
    std::shared_ptr<PrefabInstance<InstanceData>> create_instance()
    {
        auto instance = PrefabInstance<InstanceData>::create(this->shared_from_this());
        m_instances.push_back(instance);
        return instance;
    }

    /// Name of this prefab type.
    const std::string& get_name() const { return m_name; }

    /// Offset into the group's index buffer, where this prefab starts.
    size_t get_offset() const { return m_offset; }

    /// Number of indices that make up this prefab in the group.
    GLsizei get_size() const { return m_size; }

    /// Returns all instances of this prefab type.
    /// Also deletes all weak pointers to prefab instances that have gone out of scope.
    std::vector<std::shared_ptr<PrefabInstance<InstanceData>>> get_instances() const
    {
        std::vector<std::shared_ptr<PrefabInstance<InstanceData>>> result;
        result.reserve(m_instances.size());
        for (size_t i = 0; i < m_instances.size();) {
            std::shared_ptr<PrefabInstance<InstanceData>> prefab = m_instances[i].lock();
            if (prefab) {
                result.emplace_back(std::move(prefab));
                ++i;
            }
            else {
                std::swap(m_instances[i], m_instances.back());
                m_instances.pop_back();
            }
        }
        return result;
    }

    // fields ------------------------------------------------------------------------------------------------------- //
private:
    /// Name of this prefab type.
    const std::string m_name;

    /// Offset into the groups's index buffer, where this prefab starts.
    const size_t m_offset;

    /// Number of indices that make up this prefab in the group.
    const GLsizei m_size;

    /// All instances of this prefab.
    mutable std::vector<std::weak_ptr<PrefabInstance<InstanceData>>> m_instances;
};

NOTF_CLOSE_NAMESPACE
