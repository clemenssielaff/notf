#include <ostream>

#include "notf/common/msgpack.hpp"
#include "notf/meta/numeric.hpp"
#include "notf/meta/system.hpp"

// accessor ========================================================================================================= //

NOTF_OPEN_NAMESPACE

template<>
struct Accessor<MsgPack, MsgPack> {
    static void serialize(const MsgPack& pack, uint depth, std::ostream& os) { pack._serialize(depth, os); }
};

NOTF_CLOSE_NAMESPACE

// anonymous ======================================================================================================== //

namespace {
NOTF_USING_NAMESPACE;

void write_char(const uchar value, std::ostream& os) { os.put(static_cast<char>(value)); }

void write_data(const char* bytes, const size_t size, std::ostream& os)
{
    if constexpr (is_big_endian()) {
        for (size_t i = 0, end = size; i < end; ++i) {
            os.put(bytes[i]);
        }
    }
    else {
        for (size_t i = size; i > 0; --i) {
            os.put(bytes[i - 1]);
        }
    }
}

void write_data(const uchar header, const char* bytes, const size_t size, std::ostream& os)
{
    write_char(header, os);
    write_data(bytes, size, os);
}

template<typename T>
void write_data(const uchar header, const T& value, std::ostream& os)
{
    write_data(header, std::launder(reinterpret_cast<const char*>(&value)), sizeof(T), os);
}
template<typename T>
void write_data(const T& value, std::ostream& os)
{
    write_data(std::launder(reinterpret_cast<const char*>(&value)), sizeof(T), os);
}

template<typename T, class = std::enable_if_t<std::is_integral_v<T>>>
void write_uint(T value, std::ostream& os)
{
    if (value < 128) { // store as 7-bit positive integer
        return write_char(static_cast<uchar>(value), os);
    }
    else if (value <= max_value<uint8_t>()) { // store as uint8_t
        return write_data(0xcc, static_cast<uint8_t>(value), os);
    }
    else if (value <= max_value<uint16_t>()) { // store as uint16_t
        return write_data(0xcd, static_cast<uint16_t>(value), os);
    }
    else if (static_cast<size_t>(value) <= max_value<uint32_t>()) { // store as uint32_t
        return write_data(0xce, static_cast<uint32_t>(value), os);
    }
    else {
        NOTF_ASSERT(static_cast<size_t>(value) <= max_value<uint64_t>()); // store as uint64_t
        return write_data(0xcf, static_cast<uint64_t>(value), os);
    }
}

template<typename T, class = std::enable_if_t<std::is_integral_v<T> && std::is_signed_v<T>>>
void write_int(T value, std::ostream& os)
{
    if (value >= 0) {
        return write_uint(static_cast<corresponding_unsigned_t<T>>(value), os);
    }
    if (value >= -32) { // 5-bit negative integer
        return write_char(static_cast<uchar>(value), os);
    }
    else if (value >= min_value<int8_t>()) { // store as int8_t
        return write_data(0xd0, static_cast<int8_t>(value), os);
    }
    else if (value >= min_value<int16_t>()) { // store as int16_t
        return write_data(0xd1, static_cast<int16_t>(value), os);
    }
    else if (value >= min_value<int32_t>()) { // store as int32_t
        return write_data(0xd2, static_cast<int32_t>(value), os);
    }
    else {
        NOTF_ASSERT(value >= min_value<int64_t>()); // store as int64_t
        return write_data(0xd3, static_cast<int64_t>(value), os);
    }
}

void write_string(const std::string& string, std::ostream& os)
{
    const size_t size = string.size();
    if (size < 32) {
        return write_data(0xa0 | static_cast<uint8_t>(size), string.data(), size, os);
    }
    else if (size <= max_value<uint8_t>()) {
        write_data(0xd9, static_cast<uint8_t>(size), os);
        return write_data(string.data(), size, os);
    }
    else if (size <= max_value<uint16_t>()) {
        write_data(0xda, static_cast<uint16_t>(size), os);
        return write_data(string.data(), size, os);
    }
    else {
        NOTF_ASSERT((size <= max_value<uint32_t>()));
        write_data(0xdb, static_cast<uint32_t>(size), os);
        return write_data(string.data(), size, os);
    }
}

void write_binary(const MsgPack::Binary& binary, std::ostream& os)
{
    const size_t size = binary.size();
    if (size <= max_value<uint8_t>()) {
        write_data(0xc4, static_cast<uint8_t>(size), os);
        return write_data(binary.data(), size, os);
    }
    else if (size <= max_value<uint16_t>()) {
        write_data(0xc5, static_cast<uint16_t>(size), os);
        return write_data(binary.data(), size, os);
    }
    else {
        NOTF_ASSERT((size <= max_value<uint32_t>()));
        write_data(0xc6, static_cast<uint32_t>(size), os);
        return write_data(binary.data(), size, os);
    }
}

using MsgPackPrivate = Accessor<MsgPack, MsgPack>;

void write_array(uint depth, const MsgPack::Array& array, std::ostream& os)
{
    const size_t size = array.size();
    if (size <= 15) {
        write_char(0x90 | static_cast<uint8_t>(size), os);
    }
    else if (size <= max_value<uint16_t>()) {
        write_data(0xdc, static_cast<uint16_t>(size), os);
    }
    else {
        NOTF_ASSERT((size <= max_value<uint32_t>()));
        write_data(0xdd, static_cast<uint32_t>(size), os);
    }

    for (const MsgPack& element : array) {
        MsgPackPrivate::serialize(element, depth, os);
    }
}

void write_map(uint depth, const MsgPack::Map& map, std::ostream& os)
{
    const size_t size = map.size();
    if (size <= 15) {
        write_char(0x80 | static_cast<uint8_t>(size), os);
    }
    else if (size <= max_value<uint16_t>()) {
        write_data(0xde, static_cast<uint16_t>(size), os);
    }
    else {
        NOTF_ASSERT((size <= max_value<uint32_t>()));
        write_data(0xdf, static_cast<uint32_t>(size), os);
    }

    for (auto it = map.begin(), end = map.end(); it != end; ++it) {
        MsgPackPrivate::serialize(it->first, depth, os);  // key
        MsgPackPrivate::serialize(it->second, depth, os); // value
    }
}

void write_extension(const MsgPack::Extension& extension, std::ostream& os)
{
    const uint8_t type = extension.first;
    const MsgPack::Binary& binary = extension.second;
    const size_t size = binary.size();

    if (size == 1) {
        write_char(0xd4, os);
    }
    else if (size == 2) {
        write_char(0xd5, os);
    }
    else if (size == 4) {
        write_char(0xd6, os);
    }
    else if (size == 8) {
        write_char(0xd7, os);
    }
    else if (size == 16) {
        write_char(0xd8, os);
    }
    else if (size < max_value<uint8_t>()) {
        write_data(0xc7, static_cast<uint8_t>(size), os);
    }
    else if (size < max_value<uint16_t>()) {
        write_data(0xc8, static_cast<uint16_t>(size), os);
    }
    else {
        NOTF_ASSERT((size < max_value<uint32_t>()));
        write_data(0xc9, static_cast<uint32_t>(size), os);
    }

    write_data(type, binary.data(), size, os);
}

} // namespace

// msgpack ========================================================================================================== //

NOTF_OPEN_NAMESPACE

void MsgPack::_serialize(uint depth, std::ostream& os) const // TODO: limit depth?
{
    std::visit(
        overloaded{
            [&](None) { write_char(0xc0, os); },
            [&](bool value) { write_char(value ? 0xc3 : 0xc2, os); },
            [&](Int value) { write_int(value, os); },
            [&](Uint value) { write_uint(value, os); },
            [&](Real value) { write_data(0xca, value, os); },
            [&](const String& string) { write_string(string, os); },
            [&](const Binary& binary) { write_binary(binary, os); },
            [&](const Array& array) { write_array(depth + 1, array, os); },
            [&](const Map& map) { write_map(depth + 1, map, os); },
            [&](const Extension& extension) { write_extension(extension, os); },
        },
        m_value);
}

NOTF_CLOSE_NAMESPACE
