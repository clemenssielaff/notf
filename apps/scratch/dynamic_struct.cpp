#include <algorithm>
#include <iostream>
#include <variant>
#include <vector>
#include <map>
#include <string_view>
#include <unordered_set>

#include "notf/common/utf8.hpp"
#include "notf/meta/assert.hpp"
#include "notf/meta/exception.hpp"
#include "notf/meta/numeric.hpp"
#include "notf/meta/system.hpp"
#include "notf/meta/types.hpp"

NOTF_USING_NAMESPACE;

class DynStruct {

    // types ----------------------------------------------------------------------------------- //
public:
    /// Size of a word.
    using word_t = uint8_t;

    /// All types of elements.
    enum class Type : word_t { NUMBER = std::numeric_limits<word_t>::max() - 3, STRING, LIST, TUPLE };
    // TODO: the type definitions basically eat into the space available for internal pointers
    //       then again, we will never have a pointer to null or to one, and the pointed location will always be
    //       larger than the current one... I am sure there is some smart way to do this

    static constexpr const char* get_type_name(const Type type) noexcept {
        switch (type) {
        case Type::NUMBER: return "Number";
        case Type::STRING: return "String";
        case Type::LIST: return "List";
        case Type::TUPLE: return "Map";
        }
        NOTF_ASSERT(false);
        return "";
    }

private:
    /// A Dynamic Struct is made up of nested elements.
    struct Element;

    /// Layout descriptor.
    using layout_t = std::vector<word_t>;

    /// Any numer, real or integer is stored as a double.
    using number_t = double;

    /// An UTF-8 string. 
    /// Is a separate type and not a list of chars because UTF-8 characters have dynamic width.
    using string_t = utf8_string;

    /// Lists and Tuples contain child Dynamic structs.
    using children_t = std::vector<Element>;

    /// Value variant.
    using Variant = std::variant<None, number_t, string_t, children_t>;

    class Element {

    public:
        /// Type-only constructor.
        /// Used from subclasses and trivial types NUMBER and STRING.
        Element(const Type type) : m_type(type) {}

        /// NUMBER constructor.
        template<class T, class = std::enable_if_t<is_numeric_v<T>>>
        Element(T number) : m_type(Type::NUMBER), m_value(number) {
            _produce_sublayout(*this);
        }

        /// STRING constructor.
        template<class T, class = std::enable_if_t<std::is_constructible_v<std::string, T>>>
        Element(T&& string) : m_type(Type::STRING), m_value(std::forward<T>(string)) {
            _produce_sublayout(*this);
        }

        /// The name of this element.
        const std::string& get_name() const { return m_name; }

        /// (Sub-)Layout of this element.
        const layout_t& get_layout() const { return m_layout; }

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
            const children_t& children = std::get<children_t>(m_value);
            if (index >= children.size()) {
                NOTF_THROW(TypeError, "Cannot get element {} from DynamicStruct List with only {} elements", index,
                           children.size());
            }
            return children[index];
        }

        /// Index operator for maps.
        const Element& operator[](const std::string_view name) const {
            if (m_type != Type::TUPLE) {
                NOTF_THROW(TypeError, "DynamicStruct element is not a Map, but a {}", get_type_name(m_type));
            }
            for(const Element& child : std::get<children_t>(m_value)){
                if(child.get_name() == name){
                    return child;
                }
            }
            NOTF_THROW(NameError, "DynamicStruct Map does not contain an entry \"{}\"", name);
        }

    protected:
        word_t _produce_sublayout(const Element& obj) {
            switch (obj.m_type) {

            case Type::NUMBER: return to_number(Type::NUMBER);

            case Type::STRING: return to_number(Type::STRING);

            case Type::LIST: {
                NOTF_ASSERT(std::holds_alternative<children_t>(obj.m_value));
                const children_t& children = std::get<children_t>(obj.m_value);
                NOTF_ASSERT(!children.empty());
                const word_t location = m_layout.size();
                m_layout.push_back(to_number(Type::LIST));
                _produce_sublayout(children[0]);
                return location;
            }

            case Type::TUPLE: {
                NOTF_ASSERT(std::holds_alternative<children_t>(obj.m_value));
                const children_t& children = std::get<children_t>(obj.m_value);
                NOTF_ASSERT(!children.empty());
                const word_t location = m_layout.size();
                m_layout.reserve(location + children.size() + 2);
                m_layout.push_back(to_number(Type::TUPLE));
                m_layout.push_back(children.size());
                word_t itr = m_layout.size();
                for (size_t i = 0; i < children.size(); ++i) {
                    m_layout.push_back(0);
                }
                for (const Element& child : children) {
                    m_layout[itr++] = _produce_sublayout(child);
                }
                return location;
            };
            }
            return 0;
        }

        template<class T>
        void _set_value(T&& value) {
            m_value = std::forward<T>(value);
        }

        static void _set_name(Element& element, std::string name) { element.m_name = std::move(name); }

    private:
        /// Element type.
        const Type m_type;

        /// Name of this element, is only set if this element is part of a map.
        std::string m_name;

        /// User-defined value or child elements.
        Variant m_value{};

        /// (Sub-)Layout of this element.
        layout_t m_layout;
    };

    // types ----------------------------------------------------------------------------------- //
public:
    /// The valueless NUMBER type element.
    inline static const Element Number{Type::NUMBER};

    /// The valueless STRING type element.
    inline static const Element String{Type::STRING};

    struct Map : Element {
        Map(std::initializer_list<std::pair<std::string, Element>> entries) : Element(Type::TUPLE) {
            NOTF_ASSERT(entries.size() != 0);
            children_t children;
            children.reserve(entries.size());
            std::unordered_set<std::string> unique_names;
            std::transform(entries.begin(), entries.end(), std::back_inserter(children),
                           [&unique_names](std::pair<std::string, Element> entry) {
                               if(unique_names.count(entry.first)){
                                   NOTF_THROW(NotUniqueError, "Map key \"{}\", is not unique", entry.first);
                               }
                               _set_name(entry.second, std::move(entry.first));
                               return entry.second;
                           });
            _set_value(std::move(children));
            _produce_sublayout(*this);
        }
    };

    struct List : Element {

        // methods --------------------------------------------------------- //
    public:
        /// Variadic constructor.
        /// @throws XXX If the child elements do not have the same layout.
        template<class T, class... Ts, class = std::enable_if_t<all_of_type<T, Ts...>>>
        List(T&& t, Ts&&... ts) : Element(Type::LIST) {
            if constexpr (sizeof...(ts) > 0) {
                if (!_have_all_same_layout(t, ts...)) { throw std::runtime_error("OMFG"); }
            }
            _set_value(children_t{std::forward<T>(t), std::forward<Ts>(ts)...});
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

    using word_t = DynStruct::word_t;
    using Type = DynStruct::Type;

    // clang-format off
    const auto schema = DynStruct::List(
        DynStruct::Map{
            {"coords", 
                DynStruct::List(
                    DynStruct::Map{
                        {"x", DynStruct::Number},
                        {"y", DynStruct::Number}
                    }
                )
            },
            {"name", 
                DynStruct::String
            }
        }
    );
    const auto schema_value = DynStruct::List(
            DynStruct::Map{
                {"coords", 
                    DynStruct::List(
                        DynStruct::Map{
                            {"x", 0},
                            {"y", 0}
                        },
                        DynStruct::Map{
                            {"x", 1},
                            {"k", 4.8}
                        }
                    )
                },
                {"name", "Hello World"}
            }
        
    );
    // // clang-format on

    std::map<word_t, std::string_view> legend = {
        {to_number(Type::NUMBER), "Number"},
        {to_number(Type::STRING), "String"},
        {to_number(Type::LIST), "List"},
        {to_number(Type::TUPLE), "Map"},
    };

    { // schema
        std::cout << "Schema: " << std::endl;
        size_t line = 0;
        for (const word_t& word : schema.get_layout()) {
            std::cout << line++ << ": ";
            if (legend.count(word)) {
                std::cout << legend[word] << std::endl;
            } else {
                std::cout << static_cast<int>(word) << std::endl;
            }
        }
    }

    std::cout << "-------------------------------" << std::endl;
    std::cout << (schema.get_layout() == schema_value.get_layout() ? "Success" : "Failure") << std::endl;
    std::cout << "-------------------------------\n" << std::endl;

    std::cout << std::string_view(schema_value[0]["coords"][1]["k"]) << std::endl;

    return 0;
}