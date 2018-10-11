#include "notf/common/uuid.hpp"

#include <charconv>
#include <iomanip>
#include <ostream>

#ifdef NOTF_LINUX
#include <uuid/uuid.h>
#elif defined NOTF_WINDOWS
#include <objbase.h>
#elif defined NOTF_MACINTOSH
#include <CoreFoundation/CFUUID.h>
#endif

#include "fmt/ostream.h"

// system specific uuid generation ================================================================================== //

namespace {
NOTF_USING_NAMESPACE;

#ifdef NOTF_LINUX
Uuid::Bytes generate_uuid()
{
    Uuid::Bytes uuid;
    uuid_generate(uuid.data());
    return uuid;
}
#elif defined NOTF_WINDOWS
Uuid::Bytes generate_uuid()
{
    GUID guid;
    CoCreateGuid(&guid);

    Uuid::Bytes = {(guid.Data1 >> 24) & 0xff,
                   (guid.Data1 >> 16) & 0xff,
                   (guid.Data1 >> 8) & 0xff,
                   (guid.Data1) & 0xff,

                   (guid.Data2 >> 8) & 0xff,
                   (guid.Data2) & 0xff,

                   (guid.Data3 >> 8) & 0xff,
                   (guid.Data3) & 0xff,

                   guid.Data4[0],
                   guid.Data4[1],
                   guid.Data4[2],
                   guid.Data4[3],
                   guid.Data4[4],
                   guid.Data4[5],
                   guid.Data4[6],
                   guid.Data4[7]};
    return bytes;
}
#elif defined NOTF_MACINTOSH
Uuid::Bytes generate_uuid()
{
    auto newId = CFUUIDCreate(nullptr);
    auto bytes = CFUUIDGetUUIDBytes(newId);
    CFRelease(newId);
    return {{bytes.byte0, bytes.byte1, bytes.byte2, bytes.byte3, bytes.byte4, bytes.byte5, bytes.byte6, bytes.byte7,
             bytes.byte8, bytes.byte9, bytes.byte10, bytes.byte11, bytes.byte12, bytes.byte13, bytes.byte14,
             bytes.byte15}};
}
#endif

Uuid::Bytes parse_uuid(std::string_view string)
{
    if (string.size() < 36) {
        return {}; // string is too small
    }

    Uuid::Bytes bytes;
    if (std::from_chars(string.data() + 0, string.data() + 2, bytes[0], 16).ec != std::errc{}) { return {}; }
    if (std::from_chars(string.data() + 2, string.data() + 4, bytes[1], 16).ec != std::errc{}) { return {}; }
    if (std::from_chars(string.data() + 4, string.data() + 6, bytes[2], 16).ec != std::errc{}) { return {}; }
    if (std::from_chars(string.data() + 6, string.data() + 8, bytes[3], 16).ec != std::errc{}) { return {}; }

    if (std::from_chars(string.data() + 9, string.data() + 11, bytes[4], 16).ec != std::errc{}) { return {}; }
    if (std::from_chars(string.data() + 11, string.data() + 13, bytes[5], 16).ec != std::errc{}) { return {}; }

    if (std::from_chars(string.data() + 14, string.data() + 16, bytes[6], 16).ec != std::errc{}) { return {}; }
    if (std::from_chars(string.data() + 16, string.data() + 18, bytes[7], 16).ec != std::errc{}) { return {}; }

    if (std::from_chars(string.data() + 19, string.data() + 21, bytes[8], 16).ec != std::errc{}) { return {}; }
    if (std::from_chars(string.data() + 21, string.data() + 23, bytes[9], 16).ec != std::errc{}) { return {}; }

    if (std::from_chars(string.data() + 24, string.data() + 26, bytes[10], 16).ec != std::errc{}) { return {}; }
    if (std::from_chars(string.data() + 26, string.data() + 28, bytes[11], 16).ec != std::errc{}) { return {}; }
    if (std::from_chars(string.data() + 28, string.data() + 30, bytes[12], 16).ec != std::errc{}) { return {}; }
    if (std::from_chars(string.data() + 30, string.data() + 32, bytes[13], 16).ec != std::errc{}) { return {}; }
    if (std::from_chars(string.data() + 32, string.data() + 34, bytes[14], 16).ec != std::errc{}) { return {}; }
    if (std::from_chars(string.data() + 34, string.data() + 36, bytes[15], 16).ec != std::errc{}) { return {}; }
    return bytes;
}

} // namespace

// uuid ============================================================================================================= //

NOTF_OPEN_NAMESPACE

Uuid::Uuid(std::string_view string) : m_bytes(parse_uuid(std::move(string))) {}

Uuid Uuid::generate() { return generate_uuid(); }

std::string Uuid::to_string() const
{
    return fmt::format(
        "{:02x}{:02x}{:02x}{:02x}-{:02x}{:02x}-{:02x}{:02x}-{:02x}{:02x}-{:02x}{:02x}{:02x}{:02x}{:02x}{:02x}",
        m_bytes[0], m_bytes[1], m_bytes[2], m_bytes[3], m_bytes[4], m_bytes[5], m_bytes[6], m_bytes[7], m_bytes[8],
        m_bytes[9], m_bytes[10], m_bytes[11], m_bytes[12], m_bytes[13], m_bytes[14], m_bytes[15]);
}

std::ostream& operator<<(std::ostream& os, const Uuid& uuid)
{
    const Uuid::Bytes& bytes = uuid.get_data();
    fmt::print(os,
               "{:02x}{:02x}{:02x}{:02x}-{:02x}{:02x}-{:02x}{:02x}-{:02x}{:02x}-{:02x}{:02x}{:02x}{:02x}{:02x}{:02x}",
               bytes[0], bytes[1], bytes[2], bytes[3], bytes[4], bytes[5], bytes[6], bytes[7], bytes[8], bytes[9],
               bytes[10], bytes[11], bytes[12], bytes[13], bytes[14], bytes[15]);
    return os;
}

NOTF_CLOSE_NAMESPACE
