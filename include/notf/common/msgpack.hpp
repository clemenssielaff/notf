#pragma once

#include <map>
#include <string>
#include <tuple>
#include <vector>

#include "../meta/traits.hpp"
#include "./common.hpp"

NOTF_OPEN_NAMESPACE

// msgpack value ==================================================================================================== //

namespace detail {

enum class MsgPackValueType {
    NIL = 0,                   // 0
    NUMBER = 1,                // 1 => least digit is 1
    INTEGER = 2 | NUMBER,      // 3 => least two digits are 1
    INT8 = 1 << 2 | INTEGER,   // 7
    INT16 = 2 << 2 | INTEGER,  // 11
    INT32 = 3 << 2 | INTEGER,  // 15
    INT64 = 4 << 2 | INTEGER,  // 19
    UINT8 = 5 << 2 | INTEGER,  // 23
    UINT16 = 6 << 2 | INTEGER, // 27
    UINT32 = 7 << 2 | INTEGER, // 31
    UINT64 = 8 << 2 | INTEGER, // 35
    FLOAT32 = 1 << 2 | NUMBER, // 5
    FLOAT64 = 3 << 2 | NUMBER, // 9
    BOOL = 1 << 2,             // 4
    STRING = 2 << 2,           // 8
    BINARY = 3 << 2,           // 12
    ARRAY = 4 << 2,            // 16
    OBJECT = 5 << 2,           // 20
    EXTENSIO = 6 << 2,         // 24
};

struct NilValue {};
using MsgPackArray = std::vector<MsgPack>;
using MsgPackObject = std::map<MsgPack, MsgPack>;
using MsgPackBinary = std::vector<uint8_t>;
using MsgPackExtension = std::tuple<int8_t, MsgPackBinary>;

template<MsgPackValueType type>
struct msgpack_type {
    static_assert(always_false_v<type>); // can never be instantiated
};
template<>
struct msgpack_type<MsgPackValueType::NIL> {
    using type = detail::NilValue;
};
template<>
struct msgpack_type<MsgPackValueType::INT8> {
    using type = int8_t;
};
template<>
struct msgpack_type<MsgPackValueType::INT16> {
    using type = int16_t;
};
template<>
struct msgpack_type<MsgPackValueType::INT32> {
    using type = int32_t;
};
template<>
struct msgpack_type<MsgPackValueType::INT64> {
    using type = int64_t;
};
template<>
struct msgpack_type<MsgPackValueType::UINT8> {
    using type = uint8_t;
};
template<>
struct msgpack_type<MsgPackValueType::UINT16> {
    using type = uint16_t;
};
template<>
struct msgpack_type<MsgPackValueType::UINT32> {
    using type = uint32_t;
};
template<>
struct msgpack_type<MsgPackValueType::UINT64> {
    using type = uint64_t;
};
template<>
struct msgpack_type<MsgPackValueType::FLOAT32> {
    using type = float;
};
template<>
struct msgpack_type<MsgPackValueType::FLOAT64> {
    using type = double;
};
template<>
struct msgpack_type<MsgPackValueType::BOOL> {
    using type = bool;
};
template<>
struct msgpack_type<MsgPackValueType::STRING> {
    using type = std::string;
};
// BINARY,
// ARRAY,
// OBJECT,
// EXTENSION

template<MsgPackValueType type>
using msgpack_type_v = typename msgpack_type<type>::type;

} // namespace detail

class MsgPackValue {

public:
    using Type = detail::MsgPackValueType;

    using Nil = detail::NilValue;
    using Array = detail::MsgPackArray;
    using Object = detail::MsgPackObject;
    using Binary = detail::MsgPackBinary;
    using Extension = detail::MsgPackExtension;

public:
    virtual ~MsgPackValue() = default;

    virtual bool equals(const MsgPackValue* other) const = 0;
    virtual bool less(const MsgPackValue* other) const = 0;
    virtual void dump(std::ostream& os) const = 0;
    virtual Type type() const = 0;

    virtual double get_number() const { return 0; }
    virtual float get_float32() const { return 0; }
    virtual double get_float64() const { return 0; }
    virtual int32_t get_int() const { return 0; }
    virtual int8_t get_int8() const { return 0; }
    virtual int16_t get_int16() const { return 0; }
    virtual int32_t get_int32() const { return 0; }
    virtual int64_t get_int64() const { return 0; }
    virtual uint8_t get_uint8() const { return 0; }
    virtual uint16_t get_uint16() const { return 0; }
    virtual uint32_t get_uint32() const { return 0; }
    virtual uint64_t get_uint64() const { return 0; }
    virtual bool get_bool() const { return false; }
    virtual const std::string& get_string() const { return s_empty_string; }
    virtual const Array& array_items() const { return s_empty_array; }
    virtual const Object& object_items() const { return s_empty_object; }
    virtual const Binary& binary_items() const { return s_empty_binary; }
    virtual const Extension& extension_items() const { return s_empty_extension; }
    virtual const MsgPack& operator[](size_t) const { return s_empty_pack; }
    virtual const MsgPack& operator[](const std::string&) const { return s_empty_pack; }

private:
    static inline const std::string s_empty_string;
    static inline const Array s_empty_array;
    static inline const Object s_empty_object;
    static inline const Binary s_empty_binary;
    static inline const Extension s_empty_extension;
    static inline const Nil s_nil;
    static const MsgPack s_empty_pack;
};
NOTF_CLOSE_NAMESPACE
