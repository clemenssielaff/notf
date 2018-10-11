#include <functional>
#include <iostream>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "notf/common/any.hpp"
#include "notf/meta/exception.hpp"
#include "notf/meta/function.hpp"
#include "notf/meta/stringtype.hpp"
#include "notf/meta/typename.hpp"

using namespace std;
NOTF_USING_NAMESPACE;

struct Magic {
    virtual ~Magic() = default;
    virtual void trick() const = 0;
};

std::shared_ptr<Magic> cast_ice(size_t temperature)
{
    struct IceMagic : public Magic {
        IceMagic(size_t temperature) : temperature(temperature) {}
        virtual void trick() const { std::cout << "Ice magic with " << temperature << " degrees" << '\n'; }
        size_t temperature;
    };
    return std::make_shared<IceMagic>(temperature);
}

std::shared_ptr<Magic> cast_fire(size_t temperature)
{
    struct FireMagic : public Magic {
        FireMagic(size_t temperature) : temperature(temperature) {}
        void trick() const override { std::cout << "Fire magic with " << temperature << " degrees" << '\n'; }
        size_t temperature;
    };
    return std::make_shared<FireMagic>(temperature);
}

std::shared_ptr<Magic> cast_stone(double weight, bool is_evil)
{
    struct StoneMagic : public Magic {
        StoneMagic(double weight, bool is_evil) : is_evil(is_evil), weight(weight) {}
        void trick() const override
        {
            if (is_evil) { std::cout << "Evil stone magic with " << weight << " weight" << '\n'; }
            else {
                std::cout << "Good stone magic with " << weight << " weight" << '\n';
            }
        }
        long is_evil;
        double weight;
    };
    return std::make_shared<StoneMagic>(weight, is_evil);
}

struct AnyCaster {
    virtual ~AnyCaster() = default;
    template<class... Args>
    std::shared_ptr<Magic> cast(Args&&... args)
    {
        return _cast({args...});
    }

private:
    virtual std::shared_ptr<Magic> _cast(std::vector<std::any>&&) = 0;
};

template<class Func, class = std::enable_if_t<std::is_function_v<Func>>>
struct Caster : public AnyCaster {

    using traits = notf::function_traits<Func&>;
    using return_type = typename traits::return_type;
    using args_tuple = typename traits::arg_tuple;
    static constexpr const size_t arity = traits::arity;
    template<size_t I>
    using arg_type = typename traits::template arg_type<I>;

    static_assert(std::is_same_v<return_type, std::shared_ptr<Magic>>,
                  "Caster function must return a std::shared_ptr<Magic>");

    Caster(Func& function) : m_function(function) {}
    ~Caster() override = default;

    template<std::size_t I, class T>
    static bool _fill_argument(std::vector<std::any>& source, args_tuple& target)
    {
        try {
            if constexpr (std::is_integral_v<T>) { //
                std::get<I>(target) = any_integral_cast<T>(std::move(source[I]));
            }
            else if constexpr (std::is_floating_point_v<T>) {
                std::get<I>(target) = any_real_cast<T>(std::move(source[I]));
            }
            else {
                std::get<I>(target) = std::any_cast<T>(std::move(source[I]));
            }
        }
        catch (const std::bad_any_cast&) {
            NOTF_THROW(value_error, "Expected type \"{}\", got \"{}\"", type_name<T>(), type_name(source[I].type()));
        }
        return true;
    }

    template<std::size_t... I>
    static void _fill_arguments(std::index_sequence<I...>, std::vector<std::any>& source, args_tuple& target)
    {
        (_fill_argument<I, arg_type<I>>(source, target) && ...);
    }

    std::shared_ptr<Magic> _cast(std::vector<std::any>&& args) final
    {
        if (args.size() != traits::arity) {
            return {}; // wrong number of arguments
        }

        typename traits::arg_tuple typed_arguments;
        _fill_arguments(std::make_index_sequence<arity>{}, args, typed_arguments);

        return std::apply(m_function, std::move(typed_arguments));
    }

    size_t padding;
    std::function<Func> m_function;
};

struct CasterManager {

    static auto& get_the_register() // wraps the register, otherwise it doesn't get initialized correctly
    {
        static std::unordered_map<const char*, std::unique_ptr<AnyCaster>> the_register;
        return the_register;
    }

    template<class T>
    struct register_magically {
    private:
        struct exec_register {
            exec_register()
            {
                std::cout << "Registering " << T::name << std::endl;
                get_the_register().emplace(T::name, T::create());
            }
        };
        // force instantiation of definition of static member
        template<exec_register&>
        struct force_instantiation {};
        static inline exec_register register_object;
        static force_instantiation<register_object> create_instance;
    };
};

#define REGISTER_CASTER(X)                                                                              \
    struct Register##X : public CasterManager::register_magically<Register##X> {                        \
        static constexpr const char* name = #X;                                                         \
        static std::unique_ptr<AnyCaster> create() { return std::make_unique<decltype(Caster(X))>(X); } \
    }

REGISTER_CASTER(cast_fire);
REGISTER_CASTER(cast_stone);

int main()
{
//    std::cout << "Number of casters: " << CasterManager::get_the_register().size() << std::endl;
    auto& fire_caster = CasterManager::get_the_register()["cast_fire"];
    if (fire_caster) {
        fire_caster->cast(897)->trick();
    }
    else {
        std::cout << "Nope" << std::endl;
    }

    //    auto fire_caster = Caster(cast_fire);
    //    //    auto stone_caster = Caster(cast_stone);
    //    if (auto magic = fire_caster.cast(847); magic) { magic->trick(); }
    //    //    if (auto magic = stone_caster.cast(1000., false); magic) { magic->trick(); }

    return 0;
}
