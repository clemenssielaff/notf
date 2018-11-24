#include <atomic>
#include <iostream>
#include <thread>

#include "notf/app/application.hpp"
#include "notf/app/window.hpp"

#include "notf/reactive/publisher.hpp"
#include "notf/reactive/subscriber.hpp"
#include "notf/reactive/trigger.hpp"

#include "notf/app/slot.hpp"

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
        return _try_get_slot<T>(name)->get_subscriber();
    }

protected:
    template<class T>
    typename Slot<T>::publisher_t _get_slot(const std::string& name)
    {
        return _try_get_slot<T>(name)->get_publisher();
    }

    template<class T>
    SlotPtr<T> _try_get_slot(const std::string& name)
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
