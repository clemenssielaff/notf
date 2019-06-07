#include <array>
#include <cstdlib>
#include <iostream>
#include <unordered_set>
#include <vector>

#include "notf/common/variant.hpp"
#include "notf/meta/allocator.hpp"
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

    constexpr DynamicArray(DynamicArray&& other) { *this = std::move(other); }

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

    constexpr DynamicArray& operator=(DynamicArray&& other) {
        m_size = other.m_size;
        m_data = other.m_data;
        other.m_data = nullptr;
        return *this;
    }

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
    std::size_t m_size = 0;
    T* m_data = nullptr;
};

// STRUCTURED BUFFER ================================================================================================ //

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

    /// A layout describes how a particular structured buffer is structured.
    /// It is a conceptual class only, the artifact that you will work with is called a Schema which encodes a Layout.
    struct StaticLayout {

        struct Number;
        struct String;
        template<class>
        struct List;
        template<class...>
        struct Map;

        /// A static schema encodes a static Layout in a constexpr array.
        template<std::size_t size>
        using StaticSchema = std::array<std::underlying_type_t<Type>, size>;

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

        // element =============================================================== //
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
                StaticSchema<T::get_size()> schema{};
                const std::size_t actual_size = T::write_schema(schema, 0);
                NOTF_ASSERT(actual_size == T::get_size()); // if the assertion fails, this will not compile in constexpr
                return schema;
            }

        private:
            template<std::size_t... I>
            static constexpr bool _is_flat(std::index_sequence<I...>) {
                return (std::tuple_element_t<I, typename T::children_ts>::is_flat() && ...);
            }
        };

    public:
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
            static constexpr std::size_t write_schema(StaticSchema<Size>& schema, std::size_t index) noexcept {
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
            static constexpr std::size_t write_schema(StaticSchema<Size>& schema, std::size_t index) noexcept {
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
            static constexpr std::size_t write_schema(StaticSchema<size>& schema, std::size_t index) noexcept {
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
            static constexpr std::size_t write_schema(StaticSchema<Size>& schema, std::size_t index) noexcept {
                constexpr const std::size_t child_count = sizeof...(Ts);
                schema[index] = Map<Ts...>::get_id();
                schema[index + 1] = child_count;
                return _write_children(schema, index + 2, index + 2 + child_count,
                                       std::make_index_sequence<child_count>{});
            }

        private:
            template<std::size_t I, std::size_t Size>
            static constexpr void _write_child(StaticSchema<Size>& schema, const std::size_t base, std::size_t& index) {
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
            static constexpr std::size_t _write_children(StaticSchema<Size>& schema, const std::size_t base,
                                                         std::size_t index, std::index_sequence<I...>) {
                (_write_child<I>(schema, base, index), ...);
                return index;
            }
        };
    };

    struct DynamicLayout {

        // types ------------------------------------------------------------ //
    private:
        /// A value word is the size of a pointer.
        using word_t = templated_unsigned_integer_t<bitsizeof<void*>()>;

        /// A Dynamic Struct is made up of nested elements.
        struct Element;

        /// Any numer, real or integer, is stored as a floating point value in-place.
        using number_t = templated_real_t<sizeof(word_t)>;

        /// A String is a single pointer to a null-terminated, C-style string.
        using string_t = std::string;

        /// A Lists stores a variable number of array of child elements of the same type.
        using list_t = std::vector<Element>;

        /// A Map is a dynamically sized array of string, element pairs on the heap.
        using map_t = std::vector<std::pair<std::string, Element>>;

        /// Value variant.
        using Variant = std::variant<number_t, string_t, list_t, map_t>;

    public:
        /// A dynamic schema encodes a dynamic Layout in a vector.
        using DynamicSchema = std::vector<word_t, DefaultInitAllocator<word_t>>;

        // element =============================================================
    private:
        class Element {

        protected:
            Element(Variant&& value) : m_value(std::move(value)) {
                _produce_subschema(*this, m_schema);
                m_schema.shrink_to_fit();
            }

        public:
            Element() = default;

            /// Number value constructor.
            template<class T, class = std::enable_if_t<is_numeric_v<T>>>
            Element(T number) : m_value(number_t(number)) {}

            /// String value constructor.
            template<class T, class = std::enable_if_t<std::is_constructible_v<std::string, T>>>
            Element(T&& string) : m_value(string_t(std::forward<T>(string))) {}

            /// Combined schema of this element and its children.
            const DynamicSchema& get_schema() const noexcept { return m_schema; }

            /// Cast to number.
            explicit operator number_t() const {
                if (!std::holds_alternative<number_t>(m_value)) {
                    NOTF_THROW(TypeError, "DynamicStruct value is not a Number, but a {}", _get_type_name());
                }
                return std::get<number_t>(m_value);
            }

            /// Cast to string.
            explicit operator std::string_view() const {
                if (!std::holds_alternative<string_t>(m_value)) {
                    NOTF_THROW(TypeError, "DynamicStruct value is not a String, but a {}", _get_type_name());
                }
                return std::get<string_t>(m_value);
            }

            /// Index operator for lists.
            const Element& operator[](const std::size_t index) const {
                if (!std::holds_alternative<list_t>(m_value)) {
                    NOTF_THROW(TypeError, "DynamicStruct value is not a List, but a {}", _get_type_name());
                }
                const list_t& children = std::get<list_t>(m_value);
                if (index >= children.size()) {
                    NOTF_THROW(TypeError, "Cannot get element {} from DynamicStruct List with only {} elements", index,
                               children.size());
                }
                return children[index];
            }

            /// Index operator for maps.
            const Element& operator[](const std::string_view key) const {
                if (!std::holds_alternative<map_t>(m_value)) {
                    NOTF_THROW(TypeError, "DynamicStruct value is not a Map, but a {}", _get_type_name());
                }
                for (const auto& child : std::get<map_t>(m_value)) {
                    if (child.first == key) { return child.second; }
                }
                NOTF_THROW(NameError, "DynamicStruct Map does not contain an entry \"{}\"", key);
            }

            /// Value assignment.
            template<class T>
            Element& operator=(T value) {
                if constexpr (std::is_convertible_v<T, number_t>) {
                    if (std::holds_alternative<number_t>(m_value)) {
                        m_value = static_cast<number_t>(std::move(value));
                        return *this;
                    }
                } else if constexpr (std::is_convertible_v<T, string_t>) {
                    if (std::holds_alternative<string_t>(m_value)) {
                        m_value = static_cast<string_t>(std::move(value));
                        return *this;
                    }
                }
                NOTF_THROW(ValueError, "Element of type {} cannot store a \"{}\"", _get_type_name(), type_name<T>());
            }

        protected:
            const char* _get_type_name() const {
                return std::visit(overloaded{
                                      [](const number_t& number) { return get_type_name(Type::NUMBER); },
                                      [](const string_t& string) { return get_type_name(Type::STRING); },
                                      [](const list_t& list) { return get_type_name(Type::LIST); },
                                      [](const map_t& map) { return get_type_name(Type::MAP); },
                                  },
                                  m_value);
            }

            static void _produce_subschema(const Element& obj, DynamicSchema& schema) {
                std::visit(overloaded{
                               [&](const number_t&) { schema.push_back(to_number(Type::NUMBER)); },
                               [&](const string_t&) { schema.push_back(to_number(Type::STRING)); },
                               [&](const list_t& list) {
                                   NOTF_ASSERT(!list.empty());
                                   schema.push_back(to_number(Type::LIST));
                                   _produce_subschema(list[0], schema);
                               },
                               [&](const map_t& map) {
                                   NOTF_ASSERT(!map.empty());
                                   schema.reserve(schema.size() + map.size() + 2);
                                   schema.push_back(to_number(Type::MAP));
                                   schema.push_back(map.size());

                                   // pre-allocate space for the child elements
                                   std::size_t child_position = schema.size();
                                   schema.resize(child_position + map.size());

                                   for (const auto& childItr : map) {
                                       const Element& child = childItr.second;
                                       // numbers and strings are stored inline,
                                       if (std::holds_alternative<number_t>(child.m_value)) {
                                           schema[child_position++] = to_number(Type::NUMBER);
                                       } else if (std::holds_alternative<string_t>(child.m_value)) {
                                           schema[child_position++] = to_number(Type::STRING);
                                       }
                                       // lists and maps store a pointer and append themselves to after the map itself
                                       else {
                                           NOTF_ASSERT(std::holds_alternative<list_t>(child.m_value)
                                                       || std::holds_alternative<map_t>(child.m_value));
                                           schema[child_position++] = schema.size();
                                           _produce_subschema(child, schema);
                                       }
                                   }
                               },
                           },
                           obj.m_value);
            }

            // fields ------------------------------------------------------- //
        protected:
            /// User-defined value or child elements.
            Variant m_value = {};

            /// Combined schema of this element and its children.
            DynamicSchema m_schema;
        };

    public:
        struct Number : public Element {
            /// Number constructor.
            template<class T, class = std::enable_if_t<is_numeric_v<T>>>
            constexpr Number(T number) noexcept : Element(number) {}
        };

        struct String : public Element {
            /// String constructor.
            template<class T, class = std::enable_if_t<std::is_constructible_v<string_t, T>>>
            String(T&& string) : Element(std::forward<T>(string)) {}
        };

        struct List : public Element {

            // methods --------------------------------------------------------- //
        public:
            /// Variadic constructor.
            /// @throws ValueError  If the child elements do not have the same layout.
            template<class T, class... Ts, class = std::enable_if_t<all_of_type_v<T, Ts...>>>
            List(T&& t, Ts&&... ts) : Element(_produce_list(std::forward<T>(t), std::forward<Ts>(ts)...)) {}

        private:
            template<class T, class... Ts>
            static Variant _produce_list(T&& t, Ts&&... ts) {
                if constexpr (sizeof...(ts) > 0) {
                    if (!_have_all_same_layout(t, ts...)) {
                        NOTF_THROW(ValueError, "List elements must all have the same layout");
                    }
                    // TODO: check if maps all have the same keys
                }
                return list_t{std::forward<T>(t), std::forward<Ts>(ts)...};
            }

            template<class L, class R, class... Tail>
            static bool _have_all_same_layout(const L& l, const R& r, Tail&&... tail) {
                if constexpr (sizeof...(tail) == 0) {
                    return l.get_schema() == r.get_schema();
                } else {
                    if (l.get_schema() != r.get_schema()) {
                        return false;
                    } else {
                        return _have_all_same_layout(l, std::forward<Tail>(tail)...);
                    }
                }
            }
        };

        struct Map : public Element {
            /// Variadic Map constructor.
            /// param entries       Pairs of (string, Element) that make up the map.
            /// @throws ValueError  If any key is not unique.
            Map(std::initializer_list<std::pair<std::string, Element>> entries)
                : Element(_produce_map(std::move(entries))) {}

        private:
            static Variant _produce_map(std::initializer_list<std::pair<std::string, Element>>&& entries) {
                NOTF_ASSERT(entries.size() != 0);
                map_t children;
                children.reserve(entries.size());
                for (auto& entry : entries) {
                    children.push_back(std::make_pair(std::move(entry.first), std::move(entry.second)));
                }
                // map_t children(entries.begin(), entries.end());
                { // make sure that names are unique
                    std::unordered_set<std::string> unique_names;
                    for (const std::pair<std::string, Element>& child : children) {
                        if (unique_names.count(child.first)) {
                            NOTF_THROW(NotUniqueError, "Map key \"{}\", is not unique", child.first);
                        }
                        unique_names.insert(child.first);
                    }
                }
                return children;
            }
        };
    };
};

template<std::size_t Size>
constexpr bool operator==(const StructuredBuffer::StaticLayout::StaticSchema<Size>& lhs,
                          const StructuredBuffer::StaticLayout::StaticSchema<Size>& rhs) {
    for (std::size_t i = 0; i < Size; ++i) {
        if (lhs[i] != rhs[i]) { return false; }
    }
    return true;
}

namespace sbuff_static {

constexpr const StructuredBuffer::StaticLayout::Number NumberLayout;

constexpr const StructuredBuffer::StaticLayout::String StringLayout;

template<class T>
constexpr auto ListLayout(T&& t) noexcept {
    return StructuredBuffer::StaticLayout::List(std::forward<T>(t));
};

template<class... Ts>
constexpr auto MapLayout(Ts&&... ts) noexcept {
    return StructuredBuffer::StaticLayout::Map(std::forward<Ts>(ts)...);
}

template<std::size_t Size>
using Schema = StructuredBuffer::StaticLayout::StaticSchema<Size>;

using Type = StructuredBuffer::Type;

// template<std::size_t Size>
// constexpr auto Value(const Schema<Size>& schema) noexcept {
//     return StructuredBuffer::Value(schema);
// };

} // namespace sbuff_static

////////////////////////////////////////////////////////////////////////

void static_test() {
    using namespace sbuff_static;
    // clang-format off
constexpr const auto test_layout = 
    ListLayout( 
        MapLayout( 
            ListLayout( 
                MapLayout( 
                    NumberLayout,
                    StringLayout
                )
            ),
            StringLayout, 
            ListLayout(
                StringLayout
            )
        )
    );
    // clang-format on
    static_assert(!test_layout.is_flat());
    static_assert((MapLayout(NumberLayout, NumberLayout)).is_flat());
    static_assert(!(MapLayout(NumberLayout, StringLayout).is_flat()));

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
}

void dynamic_test() {
    using Map = StructuredBuffer::DynamicLayout::Map;
    using List = StructuredBuffer::DynamicLayout::List;
    // using Number = StructuredBuffer::DynamicLayout::Number;
    // using String = StructuredBuffer::DynamicLayout::String;

    // clang-format off
    // auto testmap =List (
    //     0, 1, 2, 3, 4
    // );
    // clang-format off
    auto schema = List(
        Map{
            {"coords", 
                List(
                    Map{
                        {"x", 0},
                        {"somname", "---"}
                    },
                    Map{
                        {"x", 1},
                        {"text", "Hello world"}
                    }
                )
            },
            {"name", "Hello World"},
            {"otherlist",
                List(
                    "string"
                )
            }
        }
    );
    // clang-format on
}

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

// struct Factory {
//     template<class... Args>
//     constexpr word_t List(Args... args) {
//         constexpr const std::size_t arg_count = sizeof...(Args);
//         static_assert(arg_count > 0, "Cannot create an empty list");

//         using args_ts = std::tuple<Args...>;
//         using type_t = std::tuple_element_t<0, args_ts>;
//         static_assert(all_of_type_v<type_t, Args...>, "All entries in a List must be of the same type");

//         constexpr const args_ts arguments(args...);
//         word_t* result = new word_t[1 + arg_count];
//         result[0] = arg_count;

//         for (std::size_t i = 0; i < arg_count; ++i) {
//             result[1 + i] = type_t
//         }
//     }
// };

// word_t hide_da_foos(const int number) { return notf::to_number(produce_lotta_foos(number)); }

// void delete_ma_foos(word_t as_word) { delete[](std::launder(reinterpret_cast<Foo*>(as_word))); }

int main() {
    static_test();
    dynamic_test();
    return 0;
}
