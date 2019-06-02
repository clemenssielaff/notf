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
    using layout_t = std::vector<word_t>;

    /// Buffer, storing values according to a separate Layout description.
    using buffer_t = std::vector<char, DefaultInitAllocator<char>>;

    /// Any numer, real or integer is stored as a double.
    using number_t = double;

    /// An UTF-8 string.
    /// Is a separate type and not a list of chars because UTF-8 characters have dynamic width.
    using string_t = utf8_string;

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
        /// Type-only constructor.
        Element(const Type type) : m_type(type) {
            m_value = [type]() -> decltype(m_value) {
                switch (type) {
                case Type::NUMBER: return {number_t{0}};
                case Type::STRING: return {string_t{}};
                case Type::LIST: return {list_t{}};
                case Type::MAP: return {map_t{}};
                }
                return {number_t{0}};
            }();
        }

    public:
        /// NUMBER type constructor.
        static Element Number() { return Element(Type::NUMBER); }

        /// NUMBER value constructor.
        template<class T, class = std::enable_if_t<is_numeric_v<T>>>
        Element(T number) : m_type(Type::NUMBER), m_value(number) {
            _produce_sublayout(*this);
        }

        /// STRING type constructor.
        static Element String() { return Element(Type::STRING); }

        /// STRING value constructor.
        template<class T, class = std::enable_if_t<std::is_constructible_v<std::string, T>>>
        Element(T&& string) : m_type(Type::STRING), m_value(std::forward<T>(string)) {
            _produce_sublayout(*this);
        }

        /// Combined layout of this element and its children.
        const layout_t& get_layout() const noexcept { return m_layout; }

        /// Cast to number.
        explicit operator number_t() const {
            if (m_type != Type::NUMBER) {
                NOTF_THROW(TypeError, "DynamicStruct element is not a Number, but a {}", get_type_name(m_type));
            }
            return std::get<number_t>(m_value);
        }

        /// Cast to string.
        explicit operator std::string_view() const {
            if (m_type != Type::STRING) {
                NOTF_THROW(TypeError, "DynamicStruct element is not a String, but a {}", get_type_name(m_type));
            }
            return std::get<string_t>(m_value).c_str();
        }

        /// Index operator for lists.
        const Element& operator[](const std::size_t index) const {
            if (m_type != Type::LIST) {
                NOTF_THROW(TypeError, "DynamicStruct element is not a List, but a {}", get_type_name(m_type));
            }
            const list_t& children = std::get<list_t>(m_value);
            if (index >= children.size()) {
                NOTF_THROW(TypeError, "Cannot get element {} from DynamicStruct List with only {} elements", index,
                           children.size());
            }
            return children[index];
        }
        Element& operator[](const std::size_t index) { return NOTF_FORWARD_CONST_AS_MUTABLE(operator[](index)); }

        /// Index operator for maps.
        const Element& operator[](const std::string_view key) const {
            if (m_type != Type::MAP) {
                NOTF_THROW(TypeError, "DynamicStruct element is not a Map, but a {}", get_type_name(m_type));
            }
            for (const auto& child : std::get<map_t>(m_value)) {
                if (child.first == key) { return child.second; }
            }
            NOTF_THROW(NameError, "DynamicStruct Map does not contain an entry \"{}\"", key);
        }
        Element& operator[](const std::string_view key) { return NOTF_FORWARD_CONST_AS_MUTABLE(operator[](key)); }

        /// Value assignment.
        template<class T>
        Element& operator=(T value) {
            if constexpr (std::is_convertible_v<T, number_t>) {
                if (m_type == Type::NUMBER) {
                    m_value = static_cast<number_t>(std::move(value));
                    return *this;
                }
            } else if constexpr (std::is_convertible_v<T, string_t>) {
                if (m_type == Type::STRING) {
                    m_value = static_cast<string_t>(std::move(value));
                    return *this;
                }
            }
            NOTF_THROW(ValueError, "Element of type {} cannot store a \"{}\"", get_type_name(m_type), type_name<T>());
        }

        buffer_t produce_buffer() const {
            buffer_t result;
            VectorBuffer buffer(result);
            std::ostream stream(&buffer);
            _produce_subbuffer(*this, stream);
            return {};
        }

    protected:
        word_t _produce_sublayout(const Element& obj) {
            switch (obj.m_type) {

            case Type::NUMBER: return to_number(Type::NUMBER);

            case Type::STRING: return to_number(Type::STRING);

            case Type::LIST: {
                NOTF_ASSERT(std::holds_alternative<list_t>(obj.m_value));
                const list_t& children = std::get<list_t>(obj.m_value);
                NOTF_ASSERT(!children.empty());
                const word_t location = m_layout.size();
                m_layout.push_back(to_number(Type::LIST));
                word_t itr = m_layout.size();
                m_layout.push_back(0);
                m_layout[itr] = _produce_sublayout(children[0]);
                return location;
            }

            case Type::MAP: {
                NOTF_ASSERT(std::holds_alternative<map_t>(obj.m_value));
                const map_t& children = std::get<map_t>(obj.m_value);
                NOTF_ASSERT(!children.empty());
                const word_t location = m_layout.size();
                m_layout.reserve(location + children.size() + 2);
                m_layout.push_back(to_number(Type::MAP));
                m_layout.push_back(children.size());
                word_t itr = m_layout.size();
                for (size_t i = 0; i < children.size(); ++i) {
                    m_layout.push_back(0);
                }
                for (const auto& child : children) {
                    m_layout[itr++] = _produce_sublayout(child.second);
                }
                return location;
            };
            }
            return 0;
        }

        static void _produce_subbuffer(const Element& element, std::ostream& out) {
            std::visit(overloaded{
                           [](const number_t& number) {},
                           [](const string_t& string) {},
                           [](const list_t& list) {},
                           [](const map_t& map) {},
                       },
                       element.m_value);
        }

        template<class T>
        void _set_value(T&& value) {
            m_value = std::forward<T>(value);
        }

    private:
        /// Element type.
        const Type m_type; // TODO: having both a Type and a Variant index is redundant

        /// User-defined value or child elements.
        Variant m_value{};

        /// Combined layout of this element and its children.
        layout_t m_layout;
    };

    // types ----------------------------------------------------------------------------------- //
public:
    /// The valueless NUMBER type element.
    inline static const Element Number = Element::Number();

    /// The valueless STRING type element.
    inline static const Element String = Element::String();

    struct Map : public Element {
        Map(std::initializer_list<std::pair<std::string, Element>> entries) : Element(Type::MAP) {
            NOTF_ASSERT(entries.size() != 0);
            map_t children(entries.begin(), entries.end());
            { // make sure that names are unique
                std::unordered_set<std::string> unique_names;
                for (const std::pair<std::string, Element>& child : children) {
                    if (unique_names.count(child.first)) {
                        NOTF_THROW(NotUniqueError, "Map key \"{}\", is not unique", child.first);
                    }
                    unique_names.insert(child.first);
                }
            }
            _set_value(std::move(children));
            _produce_sublayout(*this);
        }
    };

    struct List : public Element {

        // methods --------------------------------------------------------- //
    public:
        /// Variadic constructor.
        /// @throws ValueError  If the child elements do not have the same layout.
        template<class T, class... Ts, class = std::enable_if_t<all_of_type<T, Ts...>>>
        List(T&& t, Ts&&... ts) : Element(Type::LIST) {
            if constexpr (sizeof...(ts) > 0) {
                if (!_have_all_same_layout(t, ts...)) {
                    NOTF_THROW(ValueError, "List elements must all have the same layout");
                }
                // TODO: check if maps all have the same keys
            }
            _set_value(list_t{std::forward<T>(t), std::forward<Ts>(ts)...});
            _produce_sublayout(*this);
        }

    private:
        template<class L, class R, class... Tail>
        bool _have_all_same_layout(const L& l, const R& r, Tail&&... tail) {
            if constexpr (sizeof...(tail) == 0) {
                return l.get_layout() == r.get_layout();
            } else {
                if (l.get_layout() != r.get_layout()) {
                    return false;
                } else {
                    return _have_all_same_layout(l, std::forward<Tail>(tail)...);
                }
            }
        }
    };
};

// ================================================================================================================= //

int main() {

    using word_t = StructuredBuffer::word_t;
    using Type = StructuredBuffer::Type;
    using Map = StructuredBuffer::Map;
    using List = StructuredBuffer::List;
    const auto& Number = StructuredBuffer::Number;
    const auto& String = StructuredBuffer::String;

    // clang-format off
    const auto schema = List(
        Map{
            {"coords", 
                List(
                    Map{
                        {"x", Number},
                        {"y", String}
                    }
                )
            },
            {"name", 
                String
            },
            {"otherlist",
                List(
                    String
                )
            }
        }
    );
    // auto schema_value = List(
    //         Map{
    //             {"coords", 
    //                 List(
    //                     Map{
    //                         {"x", 0},
    //                         {"somname", "---"}
    //                     },
    //                     Map{
    //                         {"x", 1},
    //                         {"text", "Hello world"}
    //                     }
    //                 )
    //             },
    //             {"name", "Hello World"},
    //             {"otherlist",
    //                 List(
    //                     "string"
    //                 )
    //             }
    //         }
        
    // );
    // clang-format on

    const std::map<word_t, std::string_view> legend = {
        {to_number(Type::NUMBER), "Number"},
        {to_number(Type::STRING), "String"},
        {to_number(Type::LIST), "List"},
        {to_number(Type::MAP), "Map"},
    };

    { // schema
        std::cout << "Schema of size " << schema.get_layout().size() << ": " << std::endl;
        size_t line = 0;
        for (const word_t& word : schema.get_layout()) {
            std::cout << line++ << ": ";
            if (legend.count(word)) {
                std::cout << legend.at(word) << std::endl;
            } else {
                std::cout << static_cast<int>(word) << std::endl;
            }
        }
    }

    // std::cout << "-------------------------------" << std::endl;
    // std::cout << (schema.get_layout() == schema_value.get_layout() ? "Success" : "Failure") << std::endl;
    // std::cout << "-------------------------------\n" << std::endl;

    // schema_value[0]["coords"][1]["text"] = "Deine mudda";
    // schema_value[0]["coords"][1]["x"] = 74;
    // // std::cout << std::string_view(schema_value[0]["coords"][1]["text"]) << std::endl;
    // std::cout << double(schema_value[0]["coords"][1]["x"]) << std::endl;

    return 0;
}