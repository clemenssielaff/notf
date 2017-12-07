#pragma once

#include <algorithm>
#include <memory>
#include <vector>

#include "./gl_forwards.hpp"

namespace notf {

template<typename>
class PrefabType;

//*********************************************************************************************************************/
//*********************************************************************************************************************/

template<typename INSTANCE_DATA>
class PrefabInstance {

    template<typename>
    friend class PrefabType;

public: // types ******************************************************************************************************/
    using InstanceData = INSTANCE_DATA;

private: // methods *************************************************************************************************/
    /** Factory. */
    static std::shared_ptr<PrefabInstance<InstanceData>> create(std::shared_ptr<PrefabType<InstanceData>> prefab)
    {
#ifdef NOTF_DEBUG
        return std::shared_ptr<PrefabInstance<InstanceData>>(new PrefabInstance<InstanceData>(std::move(prefab)));
#else
        struct make_shared_enabler : public PrefabInstance<InstanceData> {
            make_shared_enabler(std::shared_ptr<PrefabType<InstanceData>> prefab)
                : PrefabInstance<InstanceData>(std::move(prefab))
            {}
        };
        return std::make_shared<make_shared_enabler>(std::move(prefab));
#endif
    }

    /** Constructor.
     * @param prefab_type   Owning reference to this instance's prefab type.
     */
    PrefabInstance(std::shared_ptr<PrefabType<InstanceData>> prefab_type) : m_prefab(std::move(prefab_type)), m_data()
    {}

public: // methods ****************************************************************************************************/
    /** Read-write access to the prefab's instance data. */
    InstanceData& data() { return m_data; }

private: // fields ****************************************************************************************************/
    /** Offset into the library's index buffer, where this prefab starts. */
    std::shared_ptr<PrefabType<InstanceData>> m_prefab;

    /** Per-instance data for this prefab. */
    InstanceData m_data;
};

//*********************************************************************************************************************/
//*********************************************************************************************************************/

/** A prefab type is an entry into a prefab group that defines how an object is rendered.
 * In order to draw a prefab type on the screen, you need to create a prefab instance of the type.
 */
template<typename INSTANCE_DATA>
class PrefabType : public std::enable_shared_from_this<PrefabType<INSTANCE_DATA>> {

    template<typename>
    friend class PrefabFactory;

    using InstanceData = INSTANCE_DATA;

protected: // methods *************************************************************************************************/
    /** Method called by the factory for creating a new prefab type. */
    static std::shared_ptr<PrefabType<InstanceData>>
    create(const std::string name, const size_t offset, const size_t size)
    {
#ifdef NOTF_DEBUG
        return std::shared_ptr<PrefabType<InstanceData>>(new PrefabType<InstanceData>(std::move(name), offset, size));
#else
        struct make_shared_enabler : public PrefabType<InstanceData> {
            make_shared_enabler(const std::string name, const size_t offset, const size_t size)
                : PrefabType<InstanceData>(std::move(name), offset, size)
            {}
        };
        return std::make_shared<make_shared_enabler>(std::move(name), offset, size);
#endif
    }

    /** Constructor.
     * @param name      Name of this prefab type.
     * @param offset    Offset into the group's index buffer, where this prefab starts.
     * @param size      Number of indices that make up this prefab in the group.
     */
    PrefabType(const std::string name, const size_t offset, const size_t size)
        : m_name(name), m_offset(offset), m_size(static_cast<GLsizei>(size)), m_instances()
    {}

public: // methods ****************************************************************************************************/
    /** Factory method. */
    std::shared_ptr<PrefabInstance<InstanceData>> create_instance()
    {
        auto instance = PrefabInstance<InstanceData>::create(this->shared_from_this());
        m_instances.push_back(instance);
        return instance;
    }

    /** Name of this prefab type. */
    const std::string& name() const { return m_name; }

    /** Offset into the group's index buffer, where this prefab starts. */
    size_t offset() const { return m_offset; }

    /** Number of indices that make up this prefab in the group. */
    GLsizei size() const { return m_size; }

    /** Returns all instances of this prefab type.
     * Also deletes all weak pointers to prefab instances that have gone out of scope.
     */
    std::vector<std::shared_ptr<PrefabInstance<InstanceData>>> instances() const
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

private: // fields ****************************************************************************************************/
    /** Name of this prefab type. */
    const std::string m_name;

    /** Offset into the groups's index buffer, where this prefab starts. */
    const size_t m_offset;

    /** Number of indices that make up this prefab in the group. */
    const GLsizei m_size;

    /** All instances of this prefab. */
    mutable std::vector<std::weak_ptr<PrefabInstance<InstanceData>>> m_instances;
};

} // namespace notf
