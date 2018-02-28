#pragma once

#include <algorithm>
#include <vector>

#include "app/forwards.hpp"
#include "common/exception.hpp"
#include "common/meta.hpp"

namespace notf {

//====================================================================================================================//

/// Baseclass for all Widget capabilities.
/// Used so that we can have common pointer type.
struct Capability {
protected:
    Capability() = default; // you shouldn't instantiate any non-derived Capabilities
};

//====================================================================================================================//

/// Convenience map for storing Capability subclasses by type.
///
/// Insert a new Capability subclass instance with:
///
///     auto my_capability = std::make_shared<MyCapability>();
///     map.insert(my_capability);
///
/// and request a given capability with:
///
///     map.get<MyCapability>();
///
/// If you insert/get something that is not a subclass of Capability, the build will fail.
/// If you try to get a capability that is not part of the map, a std::out_of_range exception is raised.
class CapabilityMap {

    // methods -------------------------------------------------------------------------------------------------------//
public:
    /// Returns a requested capability by type.
    /// @throws out_of_range    If the map does not contain the requested capability type.
    template<typename CAPABILITY, ENABLE_IF_SUBCLASS(CAPABILITY, Capability)>
    std::shared_ptr<CAPABILITY> get()
    {
        const size_t id = typeid(CAPABILITY).hash_code();
        auto it         = std::find_if(m_capabilities.begin(), m_capabilities.end(),
                               [&](const auto& candidate) { return candidate.first == id; });
        if (it == m_capabilities.end()) {
            notf_throw(out_of_range, "CapabilityMap does not contain requested Capability type");
        }
        else {
            return std::static_pointer_cast<CAPABILITY>(it.second);
        }
    }

    /// Returns a requested capability by type.
    /// If the map does not contain the requested capability, throws an std::out_of_range exception.
    template<typename CAPABILITY, ENABLE_IF_SUBCLASS(CAPABILITY, Capability)>
    std::shared_ptr<const CAPABILITY> get() const
    {
        return const_cast<CapabilityMap*>(this)->get<CAPABILITY>();
    }

    /// Inserts or replaces a capability in the map.
    /// @param capability    Capability to insert.
    template<typename CAPABILITY, ENABLE_IF_SUBCLASS(CAPABILITY, Capability)>
    void set(std::shared_ptr<CAPABILITY> capability)
    {
        const size_t id = typeid(CAPABILITY).hash_code();
        auto it         = std::find_if(m_capabilities.begin(), m_capabilities.end(),
                               [&](const auto& candidate) { return candidate.first == id; });
        if (it == m_capabilities.end()) {
            m_capabilities.emplace_back(std::make_pair(id, capability));
        }
        else {
            *it = std::make_pair(id, capability);
        }
    }

    // fields --------------------------------------------------------------------------------------------------------//
private:
    /// All capabilties by type (hash).
    std::vector<std::pair<size_t, CapabilityPtr>> m_capabilities;
};

} // namespace notf
