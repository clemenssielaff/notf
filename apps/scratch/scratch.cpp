#include <atomic>
#include <iostream>
#include <thread>

#include "notf/app/application.hpp"
#include "notf/app/window.hpp"

#include "notf/reactive/publisher.hpp"
#include "notf/reactive/subscriber.hpp"
#include "notf/reactive/trigger.hpp"

NOTF_OPEN_NAMESPACE

// int run_main(int argc, char* argv[])
//{
//    { // initialize application
//        TheApplication::Args arguments;
//        arguments.argc = argc;
//        arguments.argv = argv;
//        TheApplication::initialize(arguments);
//    }

//    auto window = Window::create();
//    return TheApplication::exec();
//}

NOTF_DECLARE_SHARED_POINTERS(struct, AnySlot);

struct AnySlot {

    /// Destructor.
    virtual ~AnySlot() = default;

    /// Name of this Slot type, for runtime reporting.
    virtual std::string_view get_type_name() const = 0;
};

template<class T>
struct Slot : public AnySlot {

    // types ----------------------------------------------------------------------------------- //
private:
    struct SlotPublisher : Publisher<T, detail::SinglePublisherPolicy> {
        void publish(const AnyPublisher* publisher, const T& value) { this->_publish(publisher, value); }
    };

    struct SlotSubscriber : Subscriber<T> {
        SlotSubscriber(Slot& slot) : m_slot(slot) {}
        void on_next(const AnyPublisher* publisher, const T& value) final
        {
            m_slot.m_publisher->publish(publisher, value);
        }
        Slot& m_slot;
    };

public:
    using value_t = T;
    using subscriber_t = std::shared_ptr<SlotSubscriber>;
    using publisher_t = std::shared_ptr<SlotPublisher>;

    // methods --------------------------------------------------------------------------------- //
public:
    Slot() : m_subscriber(std::make_shared<SlotSubscriber>(*this)), m_publisher(std::make_shared<SlotPublisher>()) {}

    /// Name of this Slot type, for runtime reporting.
    std::string_view get_type_name() const final
    {
        static const std::string my_type = type_name<T>();
        return my_type;
    }

    subscriber_t& get_subscriber() { return m_subscriber; }

    //    template<class Pub>
    //    friend std::enable_if_t<detail::is_reactive_compatible_v<std::decay_t<Pub>, subscriber_t>,
    //    Pipeline<subscriber_t>> operator|(Pub&& publisher, Slot& slot)
    //    {
    //        return std::forward<Pub>(publisher) | slot.m_subscriber;
    //    }

    template<class Sub, class DecayedSub = std::decay_t<Sub>>
    friend std::enable_if_t<detail::is_reactive_compatible_v<publisher_t, DecayedSub>, Pipeline<DecayedSub>>
    operator|(const std::shared_ptr<Slot>& slot, Sub&& subscriber)
    {
        return slot->m_publisher | std::forward<Sub>(subscriber);
    }

    // fields ---------------------------------------------------------------------------------- //
private:
    subscriber_t m_subscriber;
    publisher_t m_publisher;
};

struct Note {

public:
    Note() : m_slot(std::make_shared<Slot<int>>())
    {
        m_pipe = store_pipeline(_get_slot<int>("derbe")
                                | Trigger([](const int& value) { std::cout << value << std::endl; }));
    }

    std::string get_name() const { return "derbe"; }

    template<class T>
    typename Slot<T>::subscriber_t get_slot(const std::string& name)
    {
        return _get_slot<T>(name)->get_subscriber();
    }

protected:
    template<class T>
    std::shared_ptr<Slot<T>> _get_slot(const std::string& name)
    {
        AnySlotPtr any_slot = _get_slot(name);
        if (!any_slot) { NOTF_THROW(NameError, "Node \"{}\" has no Slot called \"{}\"", get_name(), name); }

        std::shared_ptr<Slot<T>> slot = std::dynamic_pointer_cast<Slot<T>>(any_slot);
        if (!slot) {
            NOTF_THROW(TypeError,
                       "Slot \"{}\" of Node \"{}\" is of type \"{}\", but was requested as \"{}\"", //
                       name, get_name(), any_slot->get_type_name(), type_name<T>());
        }
        return slot;
    }

private:
    AnySlotPtr _get_slot(const std::string& /*name*/) { return m_slot; }

private:
    AnySlotPtr m_slot;
    AnyPipelinePtr m_pipe;
};

int run_main(int /*argc*/, char* /*argv*/[])
{
    auto publisher = std::make_shared<Publisher<int, detail::SinglePublisherPolicy>>();
    //    auto slot = Slot<int>();
    //    auto pipe2 = slot | Trigger([](const int& value) { std::cout << value << std::endl; });

    Note node;
    auto pipe1 = publisher | node.get_slot<int>("");
//    auto pipe2 = node.slot.connect() | Trigger([](const int& value) { std::cout << value << std::endl; });

    for (int i = 0; i < 10; ++i) {
        publisher->publish(i);
    }

    return EXIT_SUCCESS;
}

NOTF_CLOSE_NAMESPACE

int main(int argc, char* argv[]) { return notf::run_main(argc, argv); }
