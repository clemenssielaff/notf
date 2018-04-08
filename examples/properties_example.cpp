#include "renderthread_example.hpp"

#include <iostream>

#include "app/application.hpp"
#include "app/property_graph.hpp"
#include "app/scene_manager.hpp"
#include "app/window.hpp"
#include "common/log.hpp"
#include "common/vector.hpp"

NOTF_USING_NAMESPACE

int properties_main(int argc, char* argv[])
{
    std::vector<int> vec {0, 1, 2, 3, 4, 5, 6, 7, 8, 9 };

    for(const auto& i : vec){
        std::cout << i << ", ";
    }
    std::cout << std::endl;

    move_behind_of(vec, iterator_at(vec, 5), iterator_at(vec, 2));

    for(const auto& i : vec){
        std::cout << i << ", ";
    }
    std::cout << std::endl;

//    Application::initialize(argc, argv);
//    auto& app = Application::instance();
//    {
//        Window::Args window_args;
//        window_args.icon = "notf.png";
//        app.create_window(window_args);
//        app.exec();
//    }

    return 0;
}
