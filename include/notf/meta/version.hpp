#pragma once

#include <cstdint>

#include "./meta.hpp"

NOTF_OPEN_META_NAMESPACE

// version ========================================================================================================== //

/// Object containing a semantic version.
struct Version {
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
        return (static_cast<uint64_t>(major) << (sizeof(minor) + sizeof(revision))) //
               | (static_cast<uint64_t>(minor) << sizeof(revision))                 //
               | static_cast<uint64_t>(revision);                                   //
    }

    const uint16_t major;
    const uint16_t minor;
    const uint32_t revision;
};

/// Version of this notf code base.
inline constexpr Version get_notf_version() noexcept
{
    return {NOTF_VERSION_MAJOR, NOTF_VERSION_MINOR, NOTF_VERSION_PATCH};
}

NOTF_CLOSE_META_NAMESPACE
