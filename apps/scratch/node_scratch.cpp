#include <iostream>
#include <string_view>
#include <tuple>

#include "notf/common/string_view.hpp"
#include "notf/meta/stringtype.hpp"

NOTF_USING_META_NAMESPACE;
NOTF_USING_COMMON_NAMESPACE;

namespace string_literal {
constexpr StringConst pos = "position";
constexpr StringConst visible = "visible";
} // namespace string_literal

// property ========================================================================================================= //

struct Position1DPropertyTrait {
    using name_t = make_string_type_t<string_literal::pos>;
    using type_t = float;
    static constexpr type_t default_value = 0.123f;
    static constexpr bool is_visible = true;
};
struct VisibilityPropertyTrait {
    using name_t = make_string_type_t<string_literal::visible>;
    using type_t = bool;
    static constexpr type_t default_value = true;
    static constexpr bool is_visible = true;
};

struct Base {
    virtual ~Base() = default;
};

template<class T>
struct TypedProperty : public Base {
    using type_t = T;

    /// Value constructor.
    TypedProperty(T value) : m_value(std::forward<T>(value)) {}

    const T& get() const noexcept { return m_value; }

    void set(T value) { m_value = std::forward<T>(value); }

    // members ------------------------------------------------------------------------------------------------------ //
protected:
    type_t m_value;
};

template<class Trait>
struct StaticProperty final : public TypedProperty<typename Trait::type_t> {

    using name_t = typename Trait::name_t;
    using type_t = typename Trait::type_t;

    static constexpr size_t hash = name_t::get_hash();

    /// Constructor.
    StaticProperty() : TypedProperty<type_t>(Trait::default_value) {}

    /// The name of this Property.
    static const char* get_name() noexcept { return Trait::name::value; }

    /// Whether a change in the Property will cause the Node to redraw or not.
    static bool is_visible() noexcept { return Trait::is_visible; }
};

// node ============================================================================================================= //

struct Node {

    /// Destructor.
    virtual ~Node() = default;

    template<class T>
    const TypedProperty<T>& get_property(std::string_view name) const
    {
        const Base* property = _get_property(std::move(name));
        if (const auto typed_property = dynamic_cast<const TypedProperty<T>*>(property)) {
            return *typed_property;
        }
        throw std::out_of_range("");
    }

    template<class T>
    TypedProperty<T>& get_property(std::string_view name)
    {
        return const_cast<TypedProperty<T>&>(const_cast<const Node*>(this)->get_property<T>(std::move(name)));
    }

protected:
    virtual const Base* _get_property(std::string_view name) const = 0;
};

struct NodeTraitExample {
    using properties = std::tuple<                //
        StaticProperty<Position1DPropertyTrait>,  //
        StaticProperty<VisibilityPropertyTrait>>; //
};

template<class Traits>
class StaticNode : public Node {

    using properties_t = typename Traits::properties;

    template<std::size_t I>
    using property_t = std::tuple_element_t<I, properties_t>;

    // methods ------------------------------------------------------------------------------------------------------ //
public:
    static constexpr auto get_property_count() noexcept { return std::tuple_size<properties_t>::value; }

    template<class Name>
    constexpr const auto& get_static_property() const noexcept
    {
        constexpr std::size_t index = _get_property_by_name<Name>();
        static_assert(index < get_property_count(), "Unknown Property");
        if constexpr (index < get_property_count()) {
            return std::get<index>(m_properties);
        }
    }
    template<class Name>
    constexpr auto& get_static_property() noexcept
    {
        const auto& const_result = const_cast<const StaticNode*>(this)->get_static_property<Name>();
        return const_cast<std::remove_const_t<decltype(const_result)>>(const_result);
    }

protected:
    const Base* _get_property(std::string_view name) const override
    {
        return _get_property_by_hash<>(hash_string(name));
    }

private:
    template<class Name, std::size_t I = 0>
    static constexpr std::size_t _get_property_by_name()
    {
        using PropertyName = typename property_t<I>::name_t;
        if constexpr (PropertyName::template is_same<Name>()) {
            return I;
        }
        if constexpr (I + 1 < get_property_count()) {
            return _get_property_by_name<Name, I + 1>();
        }
        return get_property_count();
    }

    template<std::size_t I = 0>
    const Base* _get_property_by_hash(const size_t hash_value) const
    {
        if (property_t<I>::hash == hash_value) {
            return &std::get<I>(m_properties);
        }
        if constexpr (I + 1 < get_property_count()) {
            return _get_property_by_hash<I + 1>(hash_value);
        }
        return nullptr;
    }

    // members ------------------------------------------------------------------------------------------------------ //
private:
    /// All Properties of this Node, default initialized to the Definition's default values.
    properties_t m_properties;
};

// main ============================================================================================================= //

int main()
{
    using TestNode = StaticNode<NodeTraitExample>;
    const TestNode node;

    using pos_t = make_string_type_t<string_literal::pos>;

    std::cout << node.get_property<float>("position").get() << std::endl;
    std::cout << node.get_static_property<pos_t>().get() << std::endl;

    return 0;
}
