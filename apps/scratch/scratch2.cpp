#include <iostream>
#include <memory>

#include "notf/common/thread.hpp"
NOTF_USING_NAMESPACE;

int main() {

    std::cout << "Is main thread? :" << this_thread::is_main_thread() << std::endl;
    Thread not_main;
    not_main.run([] { std::cout << "Should not be the main thread...: " << this_thread::is_main_thread() << std::endl; });

    return 0;
}
