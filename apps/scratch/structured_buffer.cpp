#include <algorithm>
#include <iostream>
#include <map>
#include <sstream>
#include <string_view>
#include <unordered_set>
#include <vector>

#include "notf/common/stream.hpp"
#include "notf/common/utf8.hpp"
#include "notf/common/variant.hpp"
#include "notf/meta/allocator.hpp"
#include "notf/meta/assert.hpp"
#include "notf/meta/exception.hpp"
#include "notf/meta/integer.hpp"
#include "notf/meta/real.hpp"
#include "notf/meta/system.hpp"
#include "notf/meta/types.hpp"

NOTF_USING_NAMESPACE;

class StructuredBuffer {

    // types ----------------------------------------------------------------------------------- //
public:
    /// A value word is the size of a pointer.
    using word_t = templated_unsigned_integer_t<bitsizeof<void*>()>;

    using layout_word_t = u_char;
    // TODO: the type definitions basically eat into the space available for internal pointers in the layout.
    //       Then again, we will never have a pointer to null or to one, and the pointed location will always be
    //       larger than the current one... I am sure there is some smart way to do this

    /// All types of elements.
    enum class Type : word_t {
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

private:
    /// A Dynamic Struct is made up of nested elements.
    struct Element;

    /// Layout description.
    using DynamicSchema = std::vector<word_t>;

    /// Buffer, storing values according to a separate Layout description.
    using buffer_t = std::vector<char, DefaultInitAllocator<char>>;

    /// Any numer, real or integer is stored as a double.
    using number_t = double;

    /// An UTF-8 string.
    /// Is a separate type and not a list of chars because UTF-8 characters have dynamic width.
    using string_t = std::string;

    /// Lists and Maps contain child Dynamic structs.
    using list_t = std::vector<Element>;

    /// Lists and Maps contain child Dynamic structs.
    using map_t = std::vector<std::pair<std::string, Element>>;

    /// Value variant.
    using Variant = std::variant<number_t, string_t, list_t, map_t>;

    // element ================================================================

    /// Base element stored in a Structured Buffer.
    /// Is the base class for specialized types Number, String, List and Map, but those only act as named constructors
    /// and they do not add any additional members or public functions.
    /// This is not a polymorphic class, you can interact with any Element like it is of any type, if you try something
    /// that won't work (like indexing a number), a corresponding exception is thrown.
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

    // types ----------------------------------------------------------------------------------- //
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

// ================================================================================================================= //

int main() {

    using word_t = StructuredBuffer::word_t;
    using Type = StructuredBuffer::Type;
    using Map = StructuredBuffer::Map;
    using List = StructuredBuffer::List;

    auto testmap =List (
        0, 1, 2, 3, 4
    );

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

    const std::map<word_t, std::string_view> legend = {
        {to_number(Type::NUMBER), "Number"},
        {to_number(Type::STRING), "String"},
        {to_number(Type::LIST), "List"},
        {to_number(Type::MAP), "Map"},
    };

    { // schema
        std::cout << "Schema of size " << schema.get_schema().size() << ": " << std::endl;
        size_t line = 0;
        for (const word_t& word : schema.get_schema()) {
            std::cout << line++ << ": ";
            if (legend.count(word)) {
                std::cout << legend.at(word) << std::endl;
            } else {
                std::cout << static_cast<int>(word) << std::endl;
            }
        }
    }

    // std::cout << "-------------------------------" << std::endl;
    // std::cout << (schema.get_schema() == schema_value.get_schema() ? "Success" : "Failure") << std::endl;
    // std::cout << "-------------------------------\n" << std::endl;

    // schema_value[0]["coords"][1]["text"] = "Deine mudda";
    // schema_value[0]["coords"][1]["x"] = 74;
    // // std::cout << std::string_view(schema_value[0]["coords"][1]["text"]) << std::endl;
    // std::cout << double(schema_value[0]["coords"][1]["x"]) << std::endl;

    return 0;
}