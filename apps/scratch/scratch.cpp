#include <array>
#include <cstdlib>
#include <iostream>

#include "notf/meta/integer.hpp"
#include "notf/meta/real.hpp"
#include "notf/meta/system.hpp"
#include "notf/meta/tuple.hpp"
#include "notf/meta/types.hpp"

NOTF_USING_NAMESPACE;

template<class T>
class DynamicArray {
public:
    using type = T;

public:
    constexpr DynamicArray() noexcept = default;

    constexpr DynamicArray(const std::size_t size) : m_size(size), m_data(new T[m_size]) {}

    template<class... Args>
    constexpr DynamicArray(const std::size_t size, Args... args) : DynamicArray(size) {
        for (std::size_t i = 0; i < m_size; ++i) {
            m_data[i] = T(args...);
        }
    }

    ~DynamicArray() noexcept {
        if (m_data) { delete[] m_data; }
    }

    constexpr std::size_t get_size() const noexcept { return m_size; }

    constexpr T& operator[](std::size_t index) { return m_data[index]; }

    constexpr const T& operator[](std::size_t index) const { return m_data[index]; }

    constexpr bool operator==(const DynamicArray& other) const noexcept {
        if (m_size != other.m_size()) { return false; }
        for (std::size_t i = 0; i < m_size; ++i) {
            if (m_data[i] != other.m_data[i]) { return false; }
        }
        return true;
    }

private:
    const std::size_t m_size = 0;
    T* m_data = nullptr;
};

struct StructuredBuffer {

    /// All types of elements in a structured buffer.
    /// Note that the underlying type of the Type enum also determines the word size of a Schema.
    /// The type must be large enough to index all expected Schemas.
    /// The largest 3 values in the type are reserved for Type identifier.
    enum class Type : uint8_t {
        _FIRST = max_v<std::underlying_type_t<Type>> - 3,
        NUMBER = _FIRST,
        STRING,
        LIST,
        MAP,
        _LAST = MAP
    };
    static_assert(to_number(Type::_LAST) == max_v<std::underlying_type_t<Type>>);

    /// Human-readable name of the types.
    static constexpr const char* get_type_name(const Type type) noexcept {
        switch (type) {
        case Type::NUMBER: return "Number";
        case Type::STRING: return "String";
        case Type::LIST: return "List";
        case Type::MAP: return "Map";
        }
        return "";
    }

    /// A schema encodes a Layout in a simple array.
    template<std::size_t size>
    using Schema = std::array<std::underlying_type_t<Type>, size>;

    /// Checks if a given type is a template specialization of Schema.
    template<class T>
    struct is_schema : std::false_type {};
    template<std::size_t size>
    struct is_schema<Schema<size>> : std::true_type {};
    template<class T>
    static constexpr bool is_schema_v = is_schema<T>::value;

    /// A layout describes how a particular structured buffer is structured.
    /// It is a conceptual class only, the artifact that you will work with is called a Schema which encodes a Layout.
    struct Layout {

        struct Number;
        struct String;
        template<class>
        struct List;
        template<class...>
        struct Map;

        /// Checks if a given type is a template specialization of List.
        template<class T>
        struct is_list : std::false_type {};
        template<class T>
        struct is_list<List<T>> : std::true_type {};
        template<class T>
        static constexpr bool is_list_v = is_list<T>::value;

        /// Checks if a given type is a template specialization of Map.
        template<class T>
        struct is_map : std::false_type {};
        template<class... Ts>
        struct is_map<Map<Ts...>> : std::true_type {};
        template<class... Ts>
        static constexpr bool is_map_v = is_map<Ts...>::value;

    private:
        template<class T, Type type>
        struct Element {
            /// Type enum value is used as layout type identifier.
            static constexpr auto get_id() noexcept { return to_number(type); }

            /// How many child elements does this type have?
            static constexpr std::size_t get_child_count() noexcept {
                return std::tuple_size_v<typename T::children_ts>;
            }

            /// A "flat" layout element does not require a pointer in the schema.
            static constexpr bool is_flat() noexcept {
                return (std::is_same_v<T, Number> || is_map_v<T>)&&_is_flat(
                    std::make_index_sequence<T::get_child_count()>{});
            }

            /// Produces the Schema with this Layout element at the root.
            static constexpr auto get_schema() noexcept {
                Schema<T::get_size()> schema{};
                const std::size_t actual_size = T::write_schema(schema, 0);
                NOTF_ASSERT(actual_size == T::get_size()); // if the assertion fails, this will not compile in constexpr
                return schema;
            }

            // static constexpr std::size_t get_buffer_size() noexcept {
            //     if constexpr(is_one_of_v<T, Number, String>){
            //         return 1;
            //     } else if constexpr(is_list_v<T>){
            //         return 1 + std::tuple_element_t<0, T::children_ts>::get_buffer_size();
            //     } else {
            //         static_assert(is_map_v<T>);
            //         return 1 + (count_type_occurrence<T::children_ts, Number, String>())
            //         return 1 + std::tuple_element_t<0, T::children_ts>::get_buffer_size();
            //     }
            // }

        private:
            template<std::size_t... I>
            static constexpr bool _is_flat(std::index_sequence<I...>) {
                return (std::tuple_element_t<I, typename T::children_ts>::is_flat() && ...);
            }
        };

    public:
        /// Any number.
        struct Number : public Element<Number, Type::NUMBER> {
            /// Numbers cannot have any child elements.
            using children_ts = std::tuple<>;

            /// Size of a Number schema is 1.
            static constexpr std::size_t get_size() noexcept { return 1; }

            /// Writes the Number ID into a schema.
            /// @param schema   Existing schema to modify.
            /// @param index    Current write index in the schema.
            /// @returns        The updated write index.
            template<std::size_t Size>
            static constexpr std::size_t write_schema(Schema<Size>& schema, std::size_t index) noexcept {
                schema[index] = get_id();
                return index + 1;
            }
        };

        struct String : public Element<String, Type::STRING> {
            /// Strings cannot have any child elements.
            using children_ts = std::tuple<>;

            /// Size of a String schema is 1.
            static constexpr std::size_t get_size() noexcept { return 1; }

            /// Writes the String ID into a schema.
            /// @param schema   Existing schema to modify.
            /// @param index    Current write index in the schema.
            /// @returns        The updated write index.
            template<std::size_t Size>
            static constexpr std::size_t write_schema(Schema<Size>& schema, std::size_t index) noexcept {
                schema[index] = get_id();
                return index + 1;
            }
        };

        template<class T>
        struct List : public Element<List<T>, Type::LIST> {
            /// Lists have a single child element.
            using children_ts = std::tuple<T>;

            /// Constexpr constructor.
            constexpr List(T) noexcept {};

            /// The size of a List schema is:
            ///     1 + n
            ///     ^   ^
            ///     |   + Size of whatever is contained in the list
            ///     + List identifier
            static constexpr std::size_t get_size() noexcept { return 1 + T::get_size(); }

            /// Writes the List ID into a schema, followed by the element description.
            /// @param schema   Existing schema to modify.
            /// @param index    Current write index in the schema.
            /// @returns        The updated write index.
            template<std::size_t size>
            static constexpr std::size_t write_schema(Schema<size>& schema, std::size_t index) noexcept {
                schema[index] = List<T>::get_id();
                return T::write_schema(schema, index + 1);
            }
        };

        template<class... Ts>
        struct Map : public Element<Map<Ts...>, Type::MAP> {
            /// Maps have an arbitrary number of child elements.
            using children_ts = std::tuple<Ts...>;

            /// Constexpr constructor.
            constexpr Map(Ts...) noexcept { static_assert(sizeof...(Ts) > 0, "Maps must have at least one element"); }

            /// The size of a Map schema is:
            ///     1 + 1 + (n - x) + m
            ///     ^   ^    ^   ^    ^
            ///     |   |    |   |    + Size of whatever is contained in the map
            ///     |   |    |   + Number of inline elements in the map
            ///     |   |    + Total number of elements in the map
            ///     |   + Element count
            ///     + Map identifier
            static constexpr std::size_t get_size() noexcept {
                return 2 + sizeof...(Ts) + (Ts::get_size() + ...)
                       - count_type_occurrence<children_ts, Number, String>();
            }

            /// Writes the Map ID into a schema, followed the size of the map and each element description.
            /// @param schema   Existing schema to modify.
            /// @param index    Current write index in the schema.
            /// @returns        The updated write index.
            template<std::size_t Size>
            static constexpr std::size_t write_schema(Schema<Size>& schema, std::size_t index) noexcept {
                constexpr const std::size_t child_count = sizeof...(Ts);
                schema[index] = Map<Ts...>::get_id();
                schema[index + 1] = child_count;
                return _write_children(schema, index + 2, index + 2 + child_count,
                                       std::make_index_sequence<child_count>{});
            }

        private:
            template<std::size_t I, std::size_t Size>
            static constexpr void _write_child(Schema<Size>& schema, const std::size_t base, std::size_t& index) {
                using child_t = std::tuple_element_t<I, children_ts>;
                if constexpr (is_one_of_v<child_t, Number, String>) {
                    // inline
                    schema[base + I] = child_t::get_id();
                } else {
                    // pointer
                    schema[base + I] = index;
                    index = child_t::write_schema(schema, index);
                }
            }

            template<std::size_t... I, std::size_t Size>
            static constexpr std::size_t _write_children(Schema<Size>& schema, const std::size_t base,
                                                         std::size_t index, std::index_sequence<I...>) {
                (_write_child<I>(schema, base, index), ...);
                return index;
            }
        };
    };

    // template<std::size_t Size>
    // static constexpr auto produce_layout(const Schema<Size>& schema) noexcept {
    //     static_assert(Size > 0, "Cannot produce a Layout from an empty Schema.");
    // }

    struct AnyValue {
        /// A value word is the size of a pointer.
        using word_t = templated_unsigned_integer_t<bitsizeof<void*>()>;

        /// Numbers are stored in-place.
        struct Number {
            /// Any numer, real or integer is stored as a floating point value.
            templated_real_t<sizeof(word_t)> value;
        };

        /// A String is a single pointer to a location on the heap.
        struct String {
            /// Strings are stored as a null-terminated, C-style string.
            const char* value;
        };

        /// Lists use two words.
        struct List {
            /// Number of elements in the list.
            word_t count;

            /// Pointer to the start of the list on the heap.
            void* ptr;
        };
    };

    // template<class S, class = std::enable_if_t<is_schema_v<S>>>
    // struct Value : private AnyValue {
    //     using schema_t = S;

    //     constexpr Value(const schema_t& schema) noexcept : m_schema(schema) {}

    // private:
    //     void * initialize_

    // private:
    //     /// Schema describing the layout of this value.
    //     const schema_t& m_schema;

    //     /// The buffer containing the data for the root element/s of the layout.
    //     void* m_buffer;
    // };

    // // template<Value::Type ValueType>
    // struct List : protected Value {
    //     word_t size = 0;
    //     word_t ptr = 0;
    // };

    // struct String : public List {};

    template<std::size_t Size>
    struct Accessor {

        Accessor(const Schema<Size>& schema, void* const buffer) : m_schema(schema), m_buffer(buffer) {}

        const Schema<Size>& m_schema;
        void* const m_buffer;
    };
};

template<std::size_t Size>
constexpr bool operator==(const StructuredBuffer::Schema<Size>& lhs, const StructuredBuffer::Schema<Size>& rhs) {
    for (std::size_t i = 0; i < Size; ++i) {
        if (lhs[i] != rhs[i]) { return false; }
    }
    return true;
}

namespace Buff {

constexpr const StructuredBuffer::Layout::Number Number;

constexpr const StructuredBuffer::Layout::String String;

template<class T>
constexpr auto List(T&& t) noexcept {
    return StructuredBuffer::Layout::List(std::forward<T>(t));
};

template<class... Ts>
constexpr auto Map(Ts&&... ts) noexcept {
    return StructuredBuffer::Layout::Map(std::forward<Ts>(ts)...);
}

template<std::size_t Size>
using Schema = StructuredBuffer::Schema<Size>;

using Type = StructuredBuffer::Type;

// template<std::size_t Size>
// constexpr auto Value(const Schema<Size>& schema) noexcept {
//     return StructuredBuffer::Value(schema);
// };

} // namespace Buff

////////////////////////////////////////////////////////////////////////

using namespace Buff;
// clang-format off
constexpr const auto test_layout = 
    List( 
        Map( 
            List( 
                Map( 
                    Number,
                    String
                )
            ),
            String, 
            List(
                String
            )
        )
    );
// clang-format on
static_assert(!test_layout.is_flat());
static_assert((Map(Number, Number)).is_flat());
static_assert(!(Map(Number, String).is_flat()));

/// Structured Data (smart approach)
/// --------------------------------
/// number of entries in list
/// pointer to list (map element 1)
///     number of entries in list
///     list entry 1, number (map element 1)
///     list entry 1, string (map element 2)
///     list entry 2, number (map element 1)
///     list entry 2, string (map element 2)
///     ...
/// string (map element 2)
/// pointer to list (map element 3)
///     number of entries in list
///     list entry 1, string
///     list entry 2, string
///     ...
/// ...

/// Structured Data (dumb approach)
/// -------------------------------
/// root pointer
/// ->  size of list
///     ->  pointer to list
///         pointer to list entry 1 (map of size 3)
///         ->  pointer to map element 1 (list)
///             ->  size of list
///                 pointer to list
///                 ->  pointer to list entry 1 (map of size 2)
///                     ->  pointer to map element 1 (number)
///                     ->  pointer to map element 2 (string)
///                     ...
///         ->  pointer to map element 2 (string)
///         ->  pointer to map element 3 (list)
///             ->  size of list
///                 pointer to list
///                 ->  pointer to list entry 1 (string)
///                     ...
///         ...

constexpr auto test_schema = test_layout.get_schema();
static_assert(test_schema.size() == 13);
static_assert(test_schema[0] == to_number(Type::LIST));
static_assert(test_schema[1] == to_number(Type::MAP));
static_assert(test_schema[2] == 3); // MapSize
static_assert(test_schema[3] == 6); // Pointer to 6
static_assert(test_schema[4] == to_number(Type::STRING));
static_assert(test_schema[5] == 11); // Pointer to 11
static_assert(test_schema[6] == to_number(Type::LIST));
static_assert(test_schema[7] == to_number(Type::MAP));
static_assert(test_schema[8] == 2); // MapSize
static_assert(test_schema[9] == to_number(Type::NUMBER));
static_assert(test_schema[10] == to_number(Type::STRING));
static_assert(test_schema[11] == to_number(Type::LIST));
static_assert(test_schema[12] == to_number(Type::STRING));

// constexpr auto test_value = Value(test_schema);

////////////////////////////////////////////////////////////////////////

// template<Value::Type type>
// void _delete_value()
// {
//     if constexpr (type == Type::NUMBER) {
//         delete[](std::launder(reinterpret_cast<typename Number::value_t*>(value_ptr)));
//     } else if constexpr (type == Type::STRING) {
//     } else if constexpr (type == Type::LIST) {
//         AnyList* any_list = std::launder(reinterpret_cast<AnyList*>(value_ptr));
//         switch(any_list->type){
//             case Type::NUMBER:
//                 delete[](reinterpret_cast<List<Type::NUMBER>*>(any_list));
//                 break;
//             case Type::STRING:
//                 delete[](reinterpret_cast<List<Type::STRING>*>(any_list));
//                 break;
//             case Type::LIST:
//                 delete[](reinterpret_cast<List<Type::LIST>*>(any_list));
//                 break;
//             case Type::MAP:
//                 delete[](reinterpret_cast<List<Type::MAP>*>(any_list));
//                 break;
//         }
//     } else if constexpr (type == Type::MAP) {
//     }
// }

// using word_t = uint64_t;
// static_assert(sizeof(void*) == sizeof(word_t));
// static_assert(sizeof(std::size_t) == sizeof(word_t));

// struct Foo {
//     ~Foo() { std::cout << "Foo " << value << " deleted" << std::endl; }
//     int value;
// };

// Foo* produce_lotta_foos(const int number) {
//     Foo* foos = new Foo[number]();
//     for (int i = 0; i < number; ++i) {
//         foos[i].value = i + 3000;
//     }
//     return foos;
// }

// word_t hide_da_foos(const int number) { return notf::to_number(produce_lotta_foos(number)); }

// void delete_ma_foos(word_t as_word) { delete[](std::launder(reinterpret_cast<Foo*>(as_word))); }

int main() {
    // auto dynamictest = List(Map(List(Map(Number, String)), String, List(String)));
    // auto wasgehtnhierab = get_layout(dynamictest);
    return 0;
}
