#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <string>

#include "notf/common/fibers.hpp"

NOTF_OPEN_NAMESPACE

// ================================================================================================================== //

int run()
{
    using channel_t =  fibers::buffered_channel< std::string >;
    try {
        channel_t chan1{  2 }, chan2{ 2 };

        fibers::fiber fping([&chan1,&chan2]{
                    chan1.push( "ping 1");
                    std::cout << chan2.value_pop() << "\n";
                    chan1.push( "ping 2");
                    std::cout << chan2.value_pop() << "\n";
                    chan1.push( "ping 3");
                    std::cout << chan2.value_pop() << "\n";
                });
        fibers::fiber fpong([&chan1,&chan2]{
                    std::cout << chan1.value_pop() << "\n";
                    chan2.push( "pong 1");
                    std::cout << chan1.value_pop() << "\n";
                    chan2.push( "pong 2");
                    std::cout << chan1.value_pop() << "\n";
                    chan2.push( "pong 3");
                });

        fping.join();
        fpong.join();

        std::cout << "very derbe, continue please!" << std::endl;

        return EXIT_SUCCESS;
    }
    catch ( std::exception const& e)
    { std::cerr << "exception: " << e.what() << std::endl; }
    catch (...)
    { std::cerr << "unhandled exception" << std::endl; }
    return EXIT_FAILURE;
}

NOTF_CLOSE_NAMESPACE

int main() {
    return notf::run();
}
