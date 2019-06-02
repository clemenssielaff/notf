#include <array>
#include <cstdlib>
#include <iostream>

#include "notf/meta/integer.hpp"
#include "notf/meta/real.hpp"
#include "notf/meta/system.hpp"
#include "notf/meta/tuple.hpp"
#include "notf/meta/types.hpp"

NOTF_USING_NAMESPACE;

struct StructuredBuffer {

    // types ----------------------------------------------------------------------------------- //
private:
    /// A layout word is a small unsigned integer.
    using layout_word_t = uint8_t;

    /// A value word is the size of a pointer.
    using value_word_t = templated_unsigned_integer_t<bitsizeof<void*>()>;

public:
    /// All types of elements in a structured buffer.
    enum class Type : layout_word_t {
        _FIRST = max_v<std::underlying_type_t<Type>> - 3,
        NUMBER = _FIRST,
        STRING,
        LIST,
        MAP,
        _LAST = MAP
    };
    static_assert(to_number(Type::_LAST) == max_v<std::underlying_type_t<Type>>);

    struct Layout {
    public:
        /// A layout word is a small unsigned integer.
        using word_t = layout_word_t;

        /// A layout buffer is a simple array of words.
        template<std::size_t size>
        using Schema = std::array<word_t, size>;

    private:
        template<class T, Type type>
        struct Element {

            /// Type enum value is used as layout type identifier.
            static constexpr word_t get_id() noexcept { return to_number(type); }

            /// Produces the Schema with this Layout element at the root.
            static constexpr auto get_schema() noexcept {
                Schema<T::get_size()> schema{};
                const std::size_t actual_size = T::write_schema(schema, 0);
                NOTF_ASSERT(actual_size == T::get_size()); // if the assertion fails, this will not compile in constexpr
                return schema;
            }
        };

    public:
        /// Any number.
        struct Number : Element<Number, Type::NUMBER> {

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

        struct String : Element<String, Type::STRING> {

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
        struct List : Element<List<T>, Type::LIST> {

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
        struct Map : Element<Map<Ts...>, Type::MAP> {

            /// Constexpr constructor.
            constexpr Map(Ts...) noexcept {}

            /// The size of a Map schema is:
            ///     1 + 1 + n + m - x
            ///     ^   ^   ^   ^   ^
            ///     |   |   |   |   + Number of inline elements
            ///     |   |   |   + Size of whatever is contained in the map
            ///     |   |   + Number of elements in the map
            ///     |   + Element count
            ///     + Map identifier
            static constexpr std::size_t get_size() noexcept {
                return 2 + sizeof...(Ts) + (Ts::get_size() + ...) - count_type_occurrence<children_t, Number, String>();
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
            using children_t = std::tuple<Ts...>;

            template<std::size_t I, std::size_t Size>
            static constexpr void _write_child(Schema<Size>& schema, const std::size_t base, std::size_t& index) {
                using child_t = std::tuple_element_t<I, children_t>;
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

    struct Value {
        /// A value word is the size of a pointer.
        using word_t = value_word_t;
    };

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

    // struct Number : protected Value {
    //     /// Any numer, real or integer is stored as a floating point value.
    //     using value_t = templated_real_t<sizeof(word_t)>;

    //     value_t value;
    // };

    // // template<Value::Type ValueType>
    // struct List : protected Value {
    //     word_t size = 0;
    //     word_t ptr = 0;
    // };

    // struct String : public List {};
};

template<std::size_t Size>
constexpr bool operator==(const StructuredBuffer::Layout::Schema<Size>& lhs,
                          const StructuredBuffer::Layout::Schema<Size>& rhs) {
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

template<size_t Size>
using Schema = StructuredBuffer::Layout::Schema<Size>;

using Type = StructuredBuffer::Type;

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
