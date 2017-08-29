#pragma once

#include <memory>
#include <typeinfo>
#include <unordered_map>

#include "common/meta.hpp"

namespace notf {

DEFINE_SHARED_POINTERS(struct, Capability);

/**********************************************************************************************************************/

/** Baseclass for all Widget capabilities.
 * Used so that we can have common pointer type.
 */
struct Capability {
protected:
    Capability() = default; // you shouldn't instantiate any non-derived Capabilities
};

/**********************************************************************************************************************/

/** Convenience map for storing Capability subclasses by type.
 *
 * Insert a new Capability subclass instance with:
 *
 *     auto my_capability = std::make_shared<MyCapability>();
 *     map.insert(my_capability);
 *
 * and request a given capability with:
 *
 *     map.get<MyCapability>();
 *
 * If you insert/get something that is not a subclass of Capability, the build will fail.
 * If you try to get a capability that is not part of the map, a std::out_of_range exception is raised.
 */
class CapabilityMap { // TODO: maybe replace internal Capability map with a vector?

public: // methods ****************************************************************************************************/
    /** Returns a requested capability by type.
     * If the map does not contain the requested capability, throws an std::out_of_range exception.
     */
    template <typename CAPABILITY, ENABLE_IF_SUBCLASS(CAPABILITY, Capability)>
    std::shared_ptr<CAPABILITY> get()
    {
        return std::static_pointer_cast<CAPABILITY>(m_capabilities.at(typeid(CAPABILITY).hash_code()));
    }

    /** Returns a requested capability by type.
     * If the map does not contain the requested capability, throws an std::out_of_range exception.
     */
    template <typename CAPABILITY, ENABLE_IF_SUBCLASS(CAPABILITY, Capability)>
    std::shared_ptr<const CAPABILITY> get() const { return const_cast<CapabilityMap*>(this)->get<CAPABILITY>(); }

    /** Inserts or replaces a capability in the map.
     * @param capability    Capability to insert.
     */
    template <typename CAPABILITY, ENABLE_IF_SUBCLASS(CAPABILITY, Capability)>
    void insert(std::shared_ptr<CAPABILITY> capability)
    {
        m_capabilities.insert({typeid(CAPABILITY).hash_code(), std::move(capability)});
    }

private: // fields ****************************************************************************************************/
    /** All capabilties by type (hash). */
    std::unordered_map<size_t, CapabilityPtr> m_capabilities;
};

} // namespace notf
