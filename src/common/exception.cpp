#include "common/exception.hpp"

#include "common/log.hpp"

namespace notf {

division_by_zero::~division_by_zero()
{
}

runtime_error::runtime_error(const std::string& message, std::string file, std::string function, uint line)
    : std::runtime_error(message)
{
    LogMessageFactory log_message(LogMessage::LEVEL::CRITICAL, line, std::move(file), std::move(function));
    log_message.input << message;
}

runtime_error::~runtime_error()
{
}

} // namespace notf
