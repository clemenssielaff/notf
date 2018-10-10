#include <istream>

#include "notf/common/msgpack.hpp"
#include "notf/meta/assert.hpp"
#include "notf/meta/bits.hpp"
#include "notf/meta/exception.hpp"
#include "notf/meta/system.hpp"

// accessor ========================================================================================================= //

NOTF_OPEN_NAMESPACE

/// Access to selected members of the MsgPack class.
template<>
struct Accessor<MsgPack, MsgPack> {
    static void serialize(const MsgPack& pack, std::ostream& os, uint depth) { pack._serialize(os, depth); }
    static MsgPack deserialize(std::istream& is, uint depth) { return MsgPack::_deserialize(is, depth); }
};

NOTF_CLOSE_NAMESPACE

// writer =========================================================================================================== //

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
    else { // untested ...
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
        MsgPackPrivate::serialize(element, os, depth);
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
        MsgPackPrivate::serialize(it->first, os, depth);  // key
        MsgPackPrivate::serialize(it->second, os, depth); // value
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

// reader =========================================================================================================== //

char read_char(std::istream& is)
{
    char result;
    is.get(result);

    if (!is.good()) {
        NOTF_THROW(MsgPack::ParseError);
    }
    return result;
}

template<class T, class = std::enable_if_t<std::is_arithmetic_v<T>>>
T read_number(std::istream& is)
{
    T result;

    if constexpr (is_big_endian()) {
        is.read(std::launder(reinterpret_cast<char*>(&result)), sizeof(T));
    }
    else { // untested ...
        for (size_t i = sizeof(T); i > 0; --i) {
            is.get(*(std::launder(reinterpret_cast<char*>(&result)) + i - 1));
        }
    }

    if (!is.good()) {
        NOTF_THROW(MsgPack::ParseError);
    }
    return result;
}

MsgPack::String read_string(std::istream& is, const uint size)
{
    MsgPack::String result(size, ' ');
    is.read(result.data(), static_cast<long>(size));

    if (!is.good()) {
        NOTF_THROW(MsgPack::ParseError);
    }
    return result;
}

MsgPack::Binary read_binary(std::istream& is, const uint size)
{
    MsgPack::Binary result(size);
    is.read(result.data(), static_cast<long>(size));

    if (!is.good()) {
        NOTF_THROW(MsgPack::ParseError);
    }
    return result;
}

MsgPack::Array read_array(std::istream& is, const uint size, const uint depth)
{
    MsgPack::Array result;
    result.reserve(size);
    for (size_t i = 0; i < size; ++i) {
        result.emplace_back(MsgPackPrivate::deserialize(is, depth));
    }
    return result;
}

MsgPack::Map read_map(std::istream& is, const uint size, const uint depth)
{
    MsgPack::Map result;
    for (size_t i = 0; i < size; ++i) {
        result.emplace(MsgPackPrivate::deserialize(is, depth), MsgPackPrivate::deserialize(is, depth));
    }
    return result;
}

MsgPack::Extension read_extension(std::istream& is, const uint size)
{
    return std::make_pair(static_cast<uint8_t>(read_char(is)), read_binary(is, size));
}

} // namespace

// msgpack ========================================================================================================== //

NOTF_OPEN_NAMESPACE

void MsgPack::_serialize(std::ostream& os, uint depth) const
{
    if (depth++ > s_max_recursion_depth) {
        NOTF_THROW(RecursionDepthExceededError);
    }

    std::visit(
        overloaded{
            [&](None) { write_char(0xc0, os); },
            [&](bool value) { write_char(value ? 0xc3 : 0xc2, os); },
            [&](Int value) { write_int(value, os); },
            [&](Uint value) { write_uint(value, os); },
            [&](Float value) { write_data(0xca, value, os); },
            [&](Double value) { write_data(0xcb, value, os); },
            [&](const String& string) { write_string(string, os); },
            [&](const Binary& binary) { write_binary(binary, os); },
            [&](const Array& array) { write_array(depth, array, os); },
            [&](const Map& map) { write_map(depth, map, os); },
            [&](const Extension& extension) { write_extension(extension, os); },
        },
        m_value);
}

MsgPack MsgPack::_deserialize(std::istream& is, uint depth)
{
    if (depth++ > s_max_recursion_depth) {
        NOTF_THROW(RecursionDepthExceededError);
    }

    const uint8_t next_byte = static_cast<uint8_t>(read_char(is));

    // none
    if (next_byte == 0xc0) {
        return {};
    }

    // bool
    else if (next_byte == 0xc2) {
        return false;
    }
    else if (next_byte == 0xc3) {
        return true;
    }

    // integer
    else if (next_byte == 0xcc) {
        return read_number<uint8_t>(is);
    }
    else if (next_byte == 0xcd) {
        return read_number<uint16_t>(is);
    }
    else if (next_byte == 0xce) {
        return read_number<uint32_t>(is);
    }
    else if (next_byte == 0xcf) {
        return read_number<uint64_t>(is);
    }
    else if (next_byte == 0xd0) {
        return read_number<int8_t>(is);
    }
    else if (next_byte == 0xd1) {
        return read_number<int16_t>(is);
    }
    else if (next_byte == 0xd2) {
        return read_number<int32_t>(is);
    }
    else if (next_byte == 0xd3) {
        return read_number<int64_t>(is);
    }

    // real
    else if (next_byte == 0xca) {
        return read_number<float>(is);
    }
    else if (next_byte == 0xcb) {
        return read_number<double>(is);
    }

    // string
    else if (next_byte == 0xd9) {
        return read_string(is, read_number<uint8_t>(is));
    }
    else if (next_byte == 0xda) {
        return read_string(is, read_number<uint16_t>(is));
    }
    else if (next_byte == 0xdb) {
        return read_string(is, read_number<uint32_t>(is));
    }

    // binary
    else if (next_byte == 0xc4) {
        return read_binary(is, read_number<uint8_t>(is));
    }
    else if (next_byte == 0xc5) {
        return read_binary(is, read_number<uint16_t>(is));
    }
    else if (next_byte == 0xc6) {
        return read_binary(is, read_number<uint32_t>(is));
    }

    // array
    else if (next_byte == 0xdc) {
        return read_array(is, read_number<uint16_t>(is), depth);
    }
    else if (next_byte == 0xdd) {
        return read_array(is, read_number<uint32_t>(is), depth);
    }

    // map
    else if (next_byte == 0xdc) {
        return read_map(is, read_number<uint16_t>(is), depth);
    }
    else if (next_byte == 0xdd) {
        return read_map(is, read_number<uint32_t>(is), depth);
    }

    // extension
    else if (next_byte == 0xd4) {
        return read_extension(is, 1);
    }
    else if (next_byte == 0xd5) {
        return read_extension(is, 2);
    }
    else if (next_byte == 0xd6) {
        return read_extension(is, 4);
    }
    else if (next_byte == 0xd7) {
        return read_extension(is, 8);
    }
    else if (next_byte == 0xd8) {
        return read_extension(is, 16);
    }
    else if (next_byte == 0xc7) {
        return read_extension(is, read_number<uint8_t>(is));
    }
    else if (next_byte == 0xc8) {
        return read_extension(is, read_number<uint16_t>(is));
    }
    else if (next_byte == 0xc9) {
        return read_extension(is, read_number<uint32_t>(is));
    }

    // fixnum
    else if (!check_bit(next_byte, 7)) {
        return static_cast<uint8_t>(next_byte);
    }
    else if (check_byte(next_byte, 0xe0)) {
        return -(static_cast<int8_t>(lowest_bits(next_byte, 5)));
    }

    // fixstr
    else if (check_byte(next_byte, 0xa0, 0x40)) {
        return read_string(is, lowest_bits(next_byte, 5));
    }

    // fixarray
    else if (check_byte(next_byte, 0x90, 0x60)) {
        return read_array(is, lowest_bits(next_byte, 4), depth);
    }

    // fixmap
    else {
        NOTF_ASSERT(check_byte(next_byte, 0x80, 0x70));
        return read_map(is, lowest_bits(next_byte, 4), depth);
    }
}

NOTF_CLOSE_NAMESPACE
