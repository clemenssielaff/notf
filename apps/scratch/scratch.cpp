#include <iostream>

#include "notf/app/property.hpp"
#include "notf/common/mnemonic.hpp"
#include "notf/common/uuid.hpp"
#include "notf/meta/hash.hpp"

NOTF_USING_NAMESPACE;

template<class T = int>
auto DefaultPublisher()
{
    return std::make_shared<Publisher<T, detail::SinglePublisherPolicy>>();
}

template<class T = int>
auto TestSubscriber()
{
    struct TestSubscriberTImpl : public Subscriber<T> {

        void on_next(const AnyPublisher*, const T& value) final { values.emplace_back(value); }

        void on_error(const AnyPublisher*, const std::exception& error) final
        {
            try {
                throw error;
            }
            catch (...) {
                exception = std::current_exception();
            };
        }

        void on_complete(const AnyPublisher*) final { is_completed = true; }

        std::vector<T> values;
        std::exception_ptr exception;
        bool is_completed = false;
        bool _padding[7];
    };
    return std::make_shared<TestSubscriberTImpl>();
}

using RTProperty = RunTimeProperty<int>;

struct PropertyPolicy {
    using value_t = int;
    static constexpr StringConst name = "position";
    static constexpr value_t default_value = 0;
    static constexpr bool is_visible = true;
};

using CTProperty = CompileTimeProperty<PropertyPolicy>;

struct MustBeSharedPtr : std::enable_shared_from_this<MustBeSharedPtr> {
    MustBeSharedPtr() { auto must_succeed = shared_from_this(); }
};

auto EverythingSubscriber()
{
    struct EverythingSubscriberImpl : public Subscriber<Everything> {
        void on_next(const AnyPublisher* /*publisher*/, ...) override { std::cout << "jup" << std::endl; }
    };
    return std::make_shared<EverythingSubscriberImpl>();
}

auto EverythingRelay() { return std::make_shared<Operator<Everything, None>>(); }

auto NoneSubscriber() {
    struct NoneSubscriber : public Subscriber<None> {
        void on_next(const AnyPublisher* /*publisher*/) override { std::cout << "got it" << std::endl; }
    };
    return std::make_shared<NoneSubscriber>();
}

int main()
{

    //    //    auto prop = std::make_shared<CTProperty>();
    //    auto prop = std::make_shared<RTProperty>("derbeprop", 42);

    //    auto publisher = DefaultPublisher();
    //    auto pipeline = prop | prop | TestSubscriber();

    //    std::cout << prop->get_name() << " " << prop->get() << '\n';
    //    publisher->publish(42);
    //    std::cout << prop->get_name() << " " << prop->get() << '\n';

    //    AnyPropertyPtr as_any = std::static_pointer_cast<AnyProperty>(prop);
    //    std::cout << fmt::format("\"{}\"", type_name<std::remove_pointer_t<decltype(as_any.get())>>()) << '\n';
    //    std::cout << fmt::format("\"{}\"", as_any->get_type_name()) << '\n';

    //    try {
    //        auto nope = MustBeSharedPtr();
    //        std::cout << "Success, I guess?" << '\n';
    //    }
    //    catch (...) {
    //        std::cout << "MustBeSharedPtr is NOT a shared_ptr" << '\n';
    //    }

    //    std::cout << "Mnemonic: " << number_to_mnemonic(hash(Uuid::generate()) % 100000000) << '\n';

//    auto subscriber = EverythingSubscriber();

//    auto int_publisher = DefaultPublisher<int>();
//    bool int_succcess = int_publisher->subscribe(subscriber);
//    int_publisher->publish(15);

//    auto float_publisher = DefaultPublisher<float>();
//    bool float_succcess = float_publisher->subscribe(subscriber);
//    float_publisher->publish(15.f);

    auto int_publisher = DefaultPublisher<int>();
    auto float_publisher = DefaultPublisher<float>();
    auto ultimate_relay = EverythingRelay();
    auto none_subscriber = NoneSubscriber();
//    int_publisher->subscribe(ultimate_relay);
//    float_publisher->subscribe(ultimate_relay);
//    ultimate_relay->subscribe(none_subscriber);
    auto pipe1 = int_publisher | ultimate_relay | none_subscriber;
    auto pipe2 = float_publisher | ultimate_relay;
    int_publisher->publish(12);
    float_publisher->publish(78.f);


    return 0; //
}
