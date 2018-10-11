#pragma once

#include <cstdint>

#include "./common.hpp"

NOTF_OPEN_NAMESPACE

// version ========================================================================================================== //

/// Object containing a semantic version.
struct Version {

    /// Value Constructor.
    constexpr Version(const uint16_t major, const uint16_t minor = 0, const uint32_t patch = 0)
        : m_major(major), m_minor(minor), m_patch(patch)
    {}

    /// Equality operator
    constexpr bool operator==(const Version& other) const noexcept { return to_number() == other.to_number(); }
    constexpr bool operator!=(const Version& other) const noexcept { return !operator==(other); }

    /// Lesser-than operator
    constexpr bool operator<(const Version& other) const noexcept { return to_number() < other.to_number(); }
    constexpr bool operator<=(const Version& other) const noexcept { return operator<(other) || operator==(other); }

    /// Greater-than operator
    constexpr bool operator>(const Version& other) const noexcept { return to_number() > other.to_number(); }
    constexpr bool operator>=(const Version& other) const noexcept { return operator>(other) || operator==(other); }

    /// Combines this version into a single 64-bit wide unsigned integer.
    constexpr uint64_t to_number() const noexcept
    {
        return (static_cast<uint64_t>(m_major) << (sizeof(m_minor) + sizeof(m_patch))) //
               | (static_cast<uint64_t>(m_minor) << sizeof(m_patch))                   //
               | static_cast<uint64_t>(m_patch);                                       //
    }

    // fields ------------------------------------------------------------------------------------------------------- //
private:
    const uint16_t m_major;
    const uint16_t m_minor;
    const uint32_t m_patch;
};

/// Version of this notf code base.
inline constexpr Version get_notf_version() noexcept
{
    return {NOTF_VERSION_MAJOR, NOTF_VERSION_MINOR, NOTF_VERSION_PATCH};
}

NOTF_CLOSE_NAMESPACE
