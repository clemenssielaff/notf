#include <iostream>
#include <string_view>
#include <tuple>

#include "notf/common/string_view.hpp"
#include "notf/meta/stringtype.hpp"

#pragma GCC diagnostic ignored "-Wweak-vtables"
#pragma GCC diagnostic ignored "-Wpadded"

NOTF_USING_META_NAMESPACE;
NOTF_USING_COMMON_NAMESPACE;

// ================================================================================================================== //

namespace string_literal {
constexpr StringConst pos = "position";
constexpr StringConst visible = "visible";
} // namespace string_literal

// property ========================================================================================================= //

struct UntypedProperty {
    virtual ~UntypedProperty() = default;
};

template<class T>
struct Property : public UntypedProperty {
    using type_t = T;

    /// Value constructor.
    Property(T value, bool is_visible) : m_value(std::forward<T>(value)), m_is_visible(is_visible) {}

    const T& get() const noexcept { return m_value; }

    void set(T value) { m_value = std::forward<T>(value); }

    /// Whether a change in the Property will cause the Node to redraw or not.
    bool is_visible() noexcept { return m_is_visible; }

    // members ------------------------------------------------------------------------------------------------------ //
protected:
    type_t m_value;

    /// Whether a change in the Property will cause the Node to redraw or not.
    bool m_is_visible;
};

template<class Trait>
struct CompileTimeProperty final : public Property<typename Trait::type_t> {

    /// Constructor.
    CompileTimeProperty() : Property<typename Trait::type_t>(Trait::default_value, Trait::is_visible) {}

    /// The name of this Property.
    static constexpr const StringConst& get_name() noexcept { return Trait::name; }

    /// Whether a change in the Property will cause the Node to redraw or not.
    static constexpr bool is_visible() noexcept { return Trait::is_visible; }

    /// Compile-time hash of the name of this Property.
    constexpr static size_t get_hash() noexcept { return Trait::name.get_hash(); }
};

// node ============================================================================================================= //

struct Node {

    /// Destructor.
    virtual ~Node() = default;

    template<class T, typename = std::enable_if_t<!std::is_same_v<T, const StringConst&>>>
    const Property<T>& get_property(std::string_view name) const
    {
        const UntypedProperty* property = _get_property(std::move(name));
        if (const auto typed_property = dynamic_cast<const Property<T>*>(property)) {
            return *typed_property;
        }
        throw std::out_of_range("");
    }

    template<class T, typename = std::enable_if_t<!std::is_same_v<T, const StringConst&>>>
    Property<T>& get_property(std::string_view name)
    {
        return const_cast<Property<T>&>(const_cast<const Node*>(this)->get_property<T>(std::move(name)));
    }

protected:
    virtual const UntypedProperty* _get_property(std::string_view name) const = 0;
};

template<class Traits>
class CompileTimeNode : public Node {

    using properties_t = typename Traits::properties;

    template<std::size_t I>
    using property_t = std::tuple_element_t<I, properties_t>;

    // methods ------------------------------------------------------------------------------------------------------ //
public:
    static constexpr auto get_property_count() noexcept { return std::tuple_size<properties_t>::value; }

    /// We are using the base class' `get_property` method in addition to the ones provided for compile time access.
    using Node::get_property;

    template<const StringConst& name>
    constexpr const auto& get_property() const noexcept
    {
        return _get_property_by_name<name, 0>();
    }

    template<const StringConst& name>
    constexpr auto& get_property() noexcept
    {
        const auto& const_result = const_cast<const CompileTimeNode*>(this)->get_property<name>();
        return const_cast<std::remove_const_t<decltype(const_result)>>(const_result);
    }

protected:
    const UntypedProperty* _get_property(std::string_view name) const override
    {
        return _get_property_by_hash<>(hash_string(name));
    }

private:
    template<const StringConst& name, std::size_t I = 0>
    constexpr const auto& _get_property_by_name() const
    {
        if constexpr (property_t<I>::get_name() == name) {
            return std::get<I>(m_properties);
        }
        else {
            static_assert(I + 1 < get_property_count(), "Unknown Property");
            return _get_property_by_name<name, I + 1>();
        }
    }

    template<std::size_t I = 0>
    const UntypedProperty* _get_property_by_hash(const size_t hash_value) const
    {
        constexpr size_t property_hash = property_t<I>::get_hash();
        if (property_hash == hash_value) {
            return &std::get<I>(m_properties);
        }
        if constexpr (I + 1 == get_property_count()) {
            return nullptr;
        }
        else {
            static_assert(I + 1 < get_property_count());
            return _get_property_by_hash<I + 1>(hash_value);
        }
    }

    // members ------------------------------------------------------------------------------------------------------ //
private:
    /// All Properties of this Node, default initialized to the Definition's default values.
    properties_t m_properties;
};

// main ============================================================================================================= //

struct Position1DPropertyTrait {
    using type_t = float;
    static constexpr StringConst name = "position";
    static constexpr type_t default_value = 0.123f;
    static constexpr bool is_visible = true;
};
struct VisibilityPropertyTrait {
    using type_t = bool;
    static constexpr StringConst name = "visible";
    static constexpr type_t default_value = true;
    static constexpr bool is_visible = true;
};

int main()
{
    struct NodeTraitExample {
        using properties = std::tuple<                     //
            CompileTimeProperty<Position1DPropertyTrait>,  //
            CompileTimeProperty<VisibilityPropertyTrait>>; //
    };
    using TestNode = CompileTimeNode<NodeTraitExample>;
    const TestNode node;

    std::cout << node.get_property<float>("position").get() << std::endl;
    std::cout << node.get_property<string_literal::pos>().get() << std::endl;
    std::cout << node.get_property<string_literal::visible>().get() << std::endl;

    return 0;
}
