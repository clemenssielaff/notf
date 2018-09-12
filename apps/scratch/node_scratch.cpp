#include <iostream>
#include <string_view>
#include <tuple>

#include "notf/common/string_view.hpp"
#include "notf/meta/stringtype.hpp"

#pragma GCC diagnostic ignored "-Wweak-vtables"
#pragma GCC diagnostic ignored "-Wpadded"

NOTF_USING_META_NAMESPACE;
NOTF_USING_COMMON_NAMESPACE;

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

    // types -------------------------------------------------------------------------------------------------------- //
protected:
    /// Tuple containing all compile time Properties of this Node type.
    using properties_t = typename Traits::properties;

    /// Type of a specific Property in this Node type.
    template<std::size_t I>
    using property_t = std::tuple_element_t<I, properties_t>;

    // methods ------------------------------------------------------------------------------------------------------ //
public:
    static constexpr auto get_property_count() noexcept { return std::tuple_size<properties_t>::value; }

    /// We are using the runtime `get_property` method in addition to the ones provided for compile time access.
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

// widget =========================================================================================================== //

namespace detail {

struct PositionPropertyTrait {
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

struct WidgetTrait {
    using properties = std::tuple<                     //
        CompileTimeProperty<PositionPropertyTrait>,    //
        CompileTimeProperty<VisibilityPropertyTrait>>; //
};

} // namespace detail

/// Base class for all Widget types.
/// We know that all Widgets share a few common Propreties at compile time. The Widget class defines the compile time
/// Properties of all Widgets, as well as a virtual interface for all other Widget types at compile or run time.
class Widget : public CompileTimeNode<::detail::WidgetTrait> {

    // types -------------------------------------------------------------------------------------------------------- //
protected:
    /// Parent Node type.
    using node_t = CompileTimeNode<::detail::WidgetTrait>;

    /// Tuple containing all compile time Properties in the parent Node type.
    using node_properties_t = node_t::properties_t;

    /// Type of a specific Property in the parent Node type.
    template<std::size_t I>
    using node_property_t = node_t::property_t<I>;

    // methods ------------------------------------------------------------------------------------------------------ //
public:
    virtual void paint() = 0;
};

template<class Traits>
class CompileTimeWidget : public Widget {

    /// Tuple containing all compile time Properties of this Widget type.
    using widget_properties_t = typename Traits::properties;

    /// Type of a specific Property in this Widget type.
    template<std::size_t I>
    using widget_property_t = std::tuple_element_t<I, widget_properties_t>;

    // methods ------------------------------------------------------------------------------------------------------ //
public:
    static constexpr auto get_property_count() noexcept
    {
        return _get_widget_property_count() + node_t::get_property_count();
    }

    /// We are using the runtime `get_property` method in addition to the ones provided for compile time access.
    using Node::get_property;

    template<const StringConst& name>
    constexpr const auto& get_property() const noexcept
    {
        return _get_widget_property_by_name<name>();
    }

    template<const StringConst& name>
    constexpr auto& get_property() noexcept
    {
        const auto& const_result = const_cast<const CompileTimeWidget*>(this)->get_property<name>();
        return const_cast<std::remove_const_t<decltype(const_result)>>(const_result);
    }

private:
    static constexpr auto _get_widget_property_count() noexcept { return std::tuple_size_v<widget_properties_t>; }

    template<const StringConst& name, std::size_t I = 0>
    constexpr const auto& _get_widget_property_by_name() const
    {
        if constexpr (widget_property_t<I>::get_name() == name) {
            return std::get<I>(m_widget_properties);
        }
        else if constexpr (I + 1 < _get_widget_property_count()) {
            return _get_widget_property_by_name<name, I + 1>();
        }
        else {
            return node_t::get_property<name>();
        }
    }

    // members ------------------------------------------------------------------------------------------------------ //
private:
    /// All Properties of this Widget, default initialized to the Trait's default values.
    widget_properties_t m_widget_properties;
};

// main ============================================================================================================= //

struct WeirdPropertyTrait {
    using type_t = int;
    static constexpr StringConst name = "soweird";
    static constexpr type_t default_value = -321;
    static constexpr bool is_visible = true;
};

int main()
{
    struct TraitExample {
        using properties = std::tuple<                //
            CompileTimeProperty<WeirdPropertyTrait>>; //
    };

    const CompileTimeNode<TraitExample> node;
    std::cout << node.get_property<int>("soweird").get() << std::endl;
    std::cout << node.get_property<WeirdPropertyTrait::name>().get() << std::endl;

    struct TestWidget : public CompileTimeWidget<TraitExample> {
        void paint() override {}
    } widget;
    std::cout << widget.get_property<float>("position").get() << std::endl;
    std::cout << widget.get_property<::detail::VisibilityPropertyTrait::name>().get() << std::endl;
    std::cout << widget.get_property<WeirdPropertyTrait::name>().get() << std::endl;

    return 0;
}
