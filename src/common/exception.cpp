#include "common/exception.hpp"

#include "common/log.hpp"

NOTF_OPEN_NAMESPACE

notf_exception::notf_exception(std::string file, std::string function, uint line, std::string message)
    : std::exception(), message(message)
{
    LogMessageFactory log_message(LogMessage::LEVEL::CRITICAL, line, std::move(file), std::move(function));
    log_message.input << message;
}

notf_exception::~notf_exception() {}

runtime_error::~runtime_error() {}

logic_error::~logic_error() {}

out_of_bounds::~out_of_bounds() {}

resource_error::~resource_error() {}

internal_error::~internal_error() {}

bad_deference_error::~bad_deference_error() {}

NOTF_CLOSE_NAMESPACE
