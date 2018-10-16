#include <iostream>

#include "notf/app/property.hpp"

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

struct PropertyPolicy {
    using value_t = int;
    static constexpr StringConst name = "position";
    static constexpr value_t default_value = 0;
    static constexpr bool is_visible = true;
};

using IProperty = CompileTimeProperty<PropertyPolicy>;

int main()
{

    auto prop = std::make_shared<IProperty>();

    auto publisher = DefaultPublisher();
    auto pipeline = publisher | prop | TestSubscriber();

    std::cout << prop->get_name() << " " << prop->get() << std::endl;
    publisher->publish(42);
    std::cout << prop->get_name() << " " << prop->get() << std::endl;
    return 0; //
}
