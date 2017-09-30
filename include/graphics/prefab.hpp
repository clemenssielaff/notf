#pragma once

#include <memory>
#include <vector>

#include "graphics/gl_forwards.hpp"

namespace notf {

template <typename>
class Prefab;

//*********************************************************************************************************************/
//*********************************************************************************************************************/

template <typename INSTANCE_DATA>
class PrefabInstance {

    template <typename>
    friend class Prefab;

public: // types ******************************************************************************************************/
    using InstanceData = INSTANCE_DATA;

private: // types *****************************************************************************************************/
    /** Factory. */
    static std::shared_ptr<PrefabInstance<InstanceData>> create(std::shared_ptr<Prefab<InstanceData>> prefab)
    {
#ifdef _DEBUG
        return std::shared_ptr<PrefabInstance<InstanceData>>(new PrefabInstance<InstanceData>(std::move(prefab)));
#else
        struct make_shared_enabler : public PrefabInstance<InstanceData> {
            make_shared_enabler(std::shared_ptr<Prefab<InstanceData>> prefab)
                : PrefabInstance<InstanceData>(std::move(prefab)) {}
        };
        return std::make_shared<make_shared_enabler>(std::move(prefab));
#endif
    }

protected: // methods *************************************************************************************************/
    /** Constructor.
     */
    PrefabInstance(std::shared_ptr<Prefab<InstanceData>> prefab)
        : m_prefab(std::move(prefab))
        , m_data()
    {
    }

public: // methods ****************************************************************************************************/
    InstanceData& data() { return m_data; }

private: // fields ****************************************************************************************************/
    /** Offset into the library's index buffer, where this prefab starts. */
    std::shared_ptr<Prefab<InstanceData>> m_prefab;

    /** Per-instance data for this prefab. */
    InstanceData m_data;
};

//*********************************************************************************************************************/
//*********************************************************************************************************************/

/** Defines a prefab type.
 *
 */
template <typename INSTANCE_DATA>
class Prefab : public std::enable_shared_from_this<Prefab<INSTANCE_DATA>> {

    template <typename>
    friend class PrefabFactory;

    using InstanceData = INSTANCE_DATA;

    /** Factory. */
    static std::shared_ptr<Prefab<InstanceData>> create(const size_t offset, const size_t size)
    {
#ifdef _DEBUG
        return std::shared_ptr<Prefab<InstanceData>>(new Prefab<InstanceData>(offset, size));
#else
        struct make_shared_enabler : public Prefab<InstanceData> {
            make_shared_enabler(const size_t offset, const size_t size)
                : Prefab<InstanceData>(offset, size) {}
        };
        return std::make_shared<make_shared_enabler>(offset, size);
#endif
    }

protected: // methods *************************************************************************************************/
    /** Constructor.
     */
    Prefab(const size_t offset, const size_t size)
        : m_offset(offset)
        , m_size(static_cast<GLsizei>(size))
        , m_instances()
    {
    }

public: // methods ****************************************************************************************************/
    std::shared_ptr<PrefabInstance<InstanceData>> create_instance()
    {
        auto instance = PrefabInstance<InstanceData>::create(this->shared_from_this());
        m_instances.push_back(instance);
        return instance;
    }

    size_t offset() const { return m_offset; }

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
                m_instances.erase(std::begin(m_instances) + i);
            }
        }
        return result;
    }

private: // fields ****************************************************************************************************/
    /** Offset into the library's index buffer, where this prefab starts. */
    const size_t m_offset;

    /** Number of indices that make up this prefab in the library. */
    const GLsizei m_size;

    /** All instances of this prefab. */
    mutable std::vector<std::weak_ptr<PrefabInstance<InstanceData>>> m_instances;
};

} // namespace notf
