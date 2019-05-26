#include <algorithm>
#include <array>
#include <cstdlib>
#include <iostream>
#include <type_traits>
#include <typeindex>
#include <typeinfo>
#include <vector>

template<class Type, size_t Arity = 1>
struct Field {
    using type_t = Type;
    static inline constexpr const size_t arity = Arity;
    const std::type_index type_index = typeid(Field<Type, Arity>);
    static_assert(Arity > 0);
};

template<class Type>
struct List {
    using type_t = Type;
};

template<class>
struct is_field : std::false_type {};
template<class T, size_t A>
struct is_field<Field<T, A>> : std::true_type {};
template<class T>
static constexpr bool is_field_v = is_field<T>::value;

template<class>
struct is_list : std::false_type {};
template<class T>
struct is_list<List<T>> : std::true_type {};
template<class T>
static constexpr bool is_list_v = is_list<T>::value;

struct V3f {
    using schema = Field<float, 3>;
    std::array<float, 3> data;
};

struct V3i {
    using schema = Field<int, 3>;
    std::array<int, 3> data;
};

struct String {
    using schema = List<char>;
    std::string data;
};

template<class, class = void>
struct has_schema : std::false_type {};
template<class T>
struct has_schema<T, std::void_t<typename T::schema>> : std::true_type {};
template<class T>
static constexpr bool has_schema_v = has_schema<T>::value;

template<class From, class To>
constexpr bool is_schema_convertible() noexcept {
    using from_t = typename From::type_t;
    using to_t = typename To::type_t;

    // fields have to have the same arity
    // for lists we can check the type only and need to defer the size check to run time
    if constexpr (is_field_v<From> && is_field_v<To>) {
        if constexpr (From::arity != To::arity) { return false; }
    }
    
    // see if the schema is convertible or not
    if constexpr (has_schema_v<from_t>) {
        if constexpr (has_schema_v<to_t>) {
            return is_schema_convertible<typename from_t::schema, typename from_t::type_t::schema>();
        } else {
            return false;
        }
    } else {
        return std::is_convertible_v<from_t, to_t>;
    }
}

template<class From, class To>
constexpr bool is_value_convertible(const From& from, const To& to) noexcept {
    // values without a schema cannot be converted automatically
    if (!has_schema_v<From> || !has_schema_v<To>) {
        return false;
    }

    // if the schemas are not compatible, the values aren't either
    else if constexpr (!is_schema_convertible<typename From::schema, typename To::schema>()) {
        return false;
    }

    // list value types need to support std::size
    else if constexpr (is_list_v<From>) {
        return std::size(from) == std::size(to);
    }

    // fallthrough
    else {
        return false;
    }
};

template<class From, class To>
bool convert_value(const From& input, To& output) {
    // do not modify the output if the conversion would not succeed
    if constexpr (!is_value_convertible(input, output)) {
        return false;
    }

    // if the values can be converted directly, do that
    else if constexpr (std::is_convertible_v<typename From::type_t, typename To::type_t>) {
        std::transform(
            std::cbegin(input), std::cend(input), std::begin(output),
            [](const typename From::type_t& value) -> typename To::type_t { return value; });
    }
}

static_assert(is_schema_convertible<V3f::schema, V3i::schema>());
static_assert(!is_schema_convertible<V3f::schema, String::schema>());
static_assert(!is_schema_convertible<String::schema, V3i::schema>());

int main() { std::cout << "Hello, Wandbox!" << std::endl; }
