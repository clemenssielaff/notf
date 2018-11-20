#include <atomic>
#include <iostream>
#include <thread>

#include "notf/app/application.hpp"
#include "notf/app/property_compiletime.hpp"
#include "notf/reactive/trigger.hpp"

NOTF_OPEN_NAMESPACE

// template<class T = int>
// NOTF_UNUSED auto DefaultPublisher()
//{
//    return std::make_shared<Publisher<T, detail::MultiPublisherPolicy>>();
//}
// int run_main(int /*argc*/, char* /*argv*/[])
//{
//    auto pub = DefaultPublisher();
//    auto pip = pub | Trigger([](const int& value) { std::cout << "(0) Got: " << value << std::endl; });
//    auto pip1 = pub | Trigger([](const AnyPublisher* publisher, const int& value) {
//                    std::cout << "(1) Got: " << value << " from publisher: " << to_number(publisher) << std::endl;
//                });
//    std::unique_ptr<AnyPipeline> pip2 = std::make_unique<decltype(pip1)>(std::move(pip1));
//    auto pip3 = pub | Trigger([](const AnyPublisher* publisher, const int& value) {
//                    std::cout << "(2) Got: " << value << " from publisher: " << to_number(publisher) << std::endl;
//                });

//    auto pip4 = store_pipeline(pub | Trigger([](const AnyPublisher* publisher, const int& value) {
//                                   std::cout << "(3) Got: " << value << " from publisher: " << to_number(publisher)
//                                             << std::endl;
//                               }));

//    pub->publish(45);
//    return EXIT_SUCCESS;
//}

int run_main(int /*argc*/, char* /*argv*/[]) { return EXIT_SUCCESS; }

// int run_main(int argc, char* argv[])
//{
//    { // initialize application
//        TheApplication::Args arguments;
//        arguments.argc = argc;
//        arguments.argv = argv;
//        TheApplication::initialize(arguments);
//    }

//    return EXIT_SUCCESS;
//}

NOTF_CLOSE_NAMESPACE

int main(int argc, char* argv[]) { return notf::run_main(argc, argv); }
