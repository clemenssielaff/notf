#pragma once

#include <cstring> // std::memcpy
#include <type_traits>

#include "common/meta.hpp"

NOTF_OPEN_NAMESPACE

/// Save bit_cast equivalent to `*reinterpret_cast<Dest*>(&source)`.
/// adpted from https://github.com/google/proto-quic/blob/master/src/base/bit_cast.h
template<typename Dest, typename Source>
inline Dest bit_cast(const Source& source)
{
    static_assert(sizeof(Dest) == sizeof(Source), "bit_cast requires source and destination to be the same size");
    static_assert(std::is_trivially_copyable<Dest>::value, "bit_cast requires the destination type to be copyable");
    static_assert(std::is_trivially_copyable<Source>::value, "bit_cast requires the source type to be copyable");
    Dest target;
    std::memcpy(&target, &source, sizeof(target));
    return target;
}

/// Like `bit_cast` but doesn't require the types to be trivially copyable.
/// Use this only if you know what you are doing.
template<typename Dest, typename Source>
inline Dest bit_cast_risky(const Source& source)
{
    static_assert(sizeof(Dest) == sizeof(Source), "bit_cast requires source and destination to be the same size");
    Dest target;
    std::memcpy(&target, &source, sizeof(target));
    return target;
}

NOTF_CLOSE_NAMESPACE
