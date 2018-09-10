#include <iostream>
#include <string_view>
#include <tuple>

#include "notf/meta/stringtype.hpp"
NOTF_USING_META_NAMESPACE;

namespace string_literal {
constexpr StringConst pos = "pos";
constexpr StringConst visible = "visible";
} // namespace string_literal

// property ========================================================================================================= //

struct Position1DPropertyTrait {
    using name = make_string_type_t<string_literal::pos>();
    using type = float;
    static constexpr type default_value = 0;
    static constexpr bool is_visible = true;
};
struct VisibilityPropertyTrait {
    using name = make_string_type_t<string_literal::visible>();
    using type = bool;
    static constexpr type default_value = true;
    static constexpr bool is_visible = true;
};

struct Property {
    virtual ~Property() = default;
};

template<class Trait>
struct TypedProperty final : public Property {
    using type = typename Trait::type;

    /// Constructor.
    constexpr TypedProperty(Trait) noexcept {}

    /// The name of this Property.
    static const char* get_name() noexcept { return Trait::name::value; }

    /// Whether a change in the Property will cause the Node to redraw or not.
    static bool is_visible() noexcept { return Trait::is_visible; }

    // members ------------------------------------------------------------------------------------------------------ //
private:
    type m_value = Trait::default_value;
};

// node ============================================================================================================= //

struct Node {

    /// Destructor.
    virtual ~Node() = default;

    virtual const Property& get_property(std::string_view name) const = 0;

    Property& get_property(std::string_view name)
    {
        return const_cast<Property&>(const_cast<const Node*>(this)->get_property(std::move(name)));
    }
};

struct NodeTraitExample {
    using properties = std::tuple<Position1DPropertyTrait, VisibilityPropertyTrait>;
};

template<class Traits>
class TypedNode : public Node {

    using properties_t = typename Traits::properties;

    template<std::size_t I>
    using property_t = std::tuple_element_t<I, properties_t>;

    // methods ------------------------------------------------------------------------------------------------------ //
public:
    static constexpr auto get_property_count() noexcept { return std::tuple_size<properties_t>::value; }

    template<class ID>
    constexpr const auto& get_property() const noexcept
    {
        constexpr std::size_t index = _get_index<ID>();
        static_assert(index < get_property_count(), "Unknown Property");
        if constexpr (index < get_property_count()) {
            return std::get<index>(m_properties);
        }
    }
    template<class ID>
    constexpr const auto& get_property(ID) const noexcept
    {
        return get_property<ID>();
    }

    template<class ID>
    constexpr auto& get_property() noexcept
    {
        constexpr std::size_t index = _get_index<ID>();
        static_assert(index < get_property_count(), "Unknown Property");
        if constexpr (index < get_property_count()) {
            return std::get<index>(m_properties);
        }
    }
    template<class ID>
    constexpr auto& get_property(ID) noexcept
    {
        return get_property<ID>();
    }

    const Property& get_property(std::string_view name) const override
    {

    }

private:
    template<class ID, std::size_t I = 0>
    static constexpr std::size_t _get_index()
    {
        using PropertyID = typename property_t<I>::static_id;
        if constexpr (PropertyID::template is_same<ID>()) {
            return I;
        }
        if constexpr (I + 1 < get_property_count()) {
            return _get_index<ID, I + 1>();
        }
        return get_property_count();
    }

    // members ------------------------------------------------------------------------------------------------------ //
private:
    /// All Properties of this Node, default initialized to the Definition's default values.
    properties_t m_properties;
};

// main ============================================================================================================= //

int main()
{
    using TestNode = TypedNode<NodeTraitExample>;
    TestNode node;

    static_assert (StringConst("ABCDEFG").get_hash() != 0);
    std::cout << StringConst("position").get_hash() << std::endl;
    std::cout << StringConst("pasition").get_hash() << std::endl;

    return 0;
}
