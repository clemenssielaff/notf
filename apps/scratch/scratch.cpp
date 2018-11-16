#include <iostream>
#include <thread>

#include "notf/common/thread.hpp"

NOTF_OPEN_NAMESPACE

void run()
{
    {
        Thread worker1(Thread::Kind::WORKER);
        Thread worker2(Thread::Kind::WORKER);
        worker1.run([]() { std::cout << to_number(this_thread::get_kind()) << std::endl; });
        worker2.run([]() { std::cout << to_number(this_thread::get_kind()) << std::endl; });
    }
    {
        Thread event(Thread::Kind::EVENT);
        event.run([]() { std::cout << to_number(this_thread::get_kind()) << std::endl; });
    }
    {
        Thread event(Thread::Kind::EVENT);
        event.run([]() { std::cout << to_number(this_thread::get_kind()) << std::endl; });
    }
}

NOTF_CLOSE_NAMESPACE

int main()
{
    notf::run();
    return 0;
}
