#if 0

#define SIGNAL_LOG_LEVEL 3

#include <string>

#include "core/application.hpp"
#include "common/log.hpp"
using namespace signal;

constexpr const char* derbe_please(ulong i)
{
    const char* derbe_please = "Derbe please";
    const char* derbe_indeed = "Derbe indeed!";

    switch(i){
    case 0:
        return derbe_please;
    default:
        return derbe_indeed;
    }
}


int main()
{
    Application& app = Application::get_instance();

    set_log_level(LogMessage::LEVEL::WARNING);

    std::string message;
    message = "Derbe indeed";

    for(ulong i = 0; i < 1000000000; ++i){
        log_debug << message;
    }

    return 0;
}
#endif
