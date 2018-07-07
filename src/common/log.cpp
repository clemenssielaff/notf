#include "common/log.hpp"

#include <cassert>

#include "common/integer.hpp"

namespace { // anonymous
NOTF_USING_NAMESPACE;

/// Constructs a terminal color prefix for the given color.
inline std::string color_prefix(ushort color) { return "\033[38;5;" + std::to_string(color) + "m"; }

} // namespace

NOTF_OPEN_NAMESPACE

LogHandler::LogHandler(size_t initial_buffer, ulong flush_interval)
    : m_write_buffer()
    , m_read_buffer()
    , m_mutex()
    , m_thread()
    , m_log_count(0)
    , m_flush_interval{flush_interval}
    , m_number_padding(2)
    , m_color_format(46)
    , m_color_trace(242)
    , m_color_info(249)
    , m_color_warning(227)
    , m_color_critical(215)
    , m_color_fatal(196)
{
    m_write_buffer.reserve(initial_buffer);
    m_read_buffer.reserve(initial_buffer);
}

void LogHandler::start()
{
    // if the handler is already running, just return
    if (m_is_running.test_and_set()) {
        return;
    }

    // otherwise, start the thread
    m_thread = std::thread(&LogHandler::run, this);
}

void LogHandler::join()
{
    if (m_thread.joinable()) {
        stop();
        m_thread.join();
    }

    // just in case, to sure that all messages are flushed
    flush_buffer(m_read_buffer);
    flush_buffer(m_write_buffer);
}

void LogHandler::set_color(LogMessage::LEVEL level, uchar color)
{
    switch (level) {
    case LogMessage::LEVEL::FORMAT:
        m_color_format = color;
        break;

    case LogMessage::LEVEL::TRACE:
        m_color_trace = color;
        break;

    case LogMessage::LEVEL::INFO:
        m_color_info = color;
        break;

    case LogMessage::LEVEL::WARNING:
        m_color_warning = color;
        break;

    case LogMessage::LEVEL::CRITICAL:
        m_color_critical = color;
        break;

    case LogMessage::LEVEL::FATAL:
        m_color_fatal = color;
        break;

    case LogMessage::LEVEL::ALL:
        m_color_format   = color;
        m_color_trace    = color;
        m_color_info     = color;
        m_color_warning  = color;
        m_color_critical = color;
        m_color_fatal    = color;
        break;

    case LogMessage::LEVEL::NONE:
        break;
    }
}

void LogHandler::run()
{
    // loop until the 'running' flag is cleared
    while (m_is_running.test_and_set()) {
        std::this_thread::sleep_for(m_flush_interval);

        // swap the buffers
        m_mutex.lock();
        std::swap(m_write_buffer, m_read_buffer); // never fails
        m_mutex.unlock();

        // flush the read buffer
        flush_buffer(m_read_buffer);
    }

    // finally, flush the write buffer as well - this is safe because no new messages are accepted any more
    flush_buffer(m_write_buffer);

    // the handler can be re-activated after a succesfull stop
    m_is_running.clear();
}

void LogHandler::flush_buffer(std::vector<LogMessage>& buffer)
{
    static const std::string TRACE    = "trace: ";
    static const std::string INFO     = "info:  ";
    static const std::string WARNING  = "warn:  ";
    static const std::string CRITICAL = "crit:  ";
    static const std::string FATAL    = "fatal: ";

    for (const LogMessage& log_message : buffer) {

        // noop messages
        if (log_message.level == LogMessage::LEVEL::NONE) {
            continue;
        }

        // formatting messages do not increate the counter, nor do they have additional info
        if (log_message.level == LogMessage::LEVEL::FORMAT) {
            std::cout << color_prefix(m_color_format) << log_message.message << "\033[0m\n";
            continue;
        }

        // padding
        ++m_log_count;
        for (auto i = m_number_padding; i > count_digits(m_log_count); --i) {
            std::cout << " ";
        }

        // prefix
        switch (log_message.level) {
        case LogMessage::LEVEL::ALL:
        case LogMessage::LEVEL::TRACE:
            std::cout << color_prefix(m_color_trace) << m_log_count << ". " << TRACE;
            break;
        case LogMessage::LEVEL::INFO:
            std::cout << color_prefix(m_color_info) << m_log_count << ". " << INFO;
            break;
        case LogMessage::LEVEL::WARNING:
            std::cout << color_prefix(m_color_warning) << m_log_count << ". " << WARNING;
            break;
        case LogMessage::LEVEL::CRITICAL:
            std::cout << color_prefix(m_color_critical) << m_log_count << ". " << CRITICAL;
            break;
        case LogMessage::LEVEL::FATAL:
            std::cout << color_prefix(m_color_fatal) << m_log_count << ". " << FATAL;
            break;
        case LogMessage::LEVEL::FORMAT:
        case LogMessage::LEVEL::NONE:
            assert(0); // should be unreachable
            break;
        }

        // message
        std::cout << log_message.message << " (from '" << log_message.caller << "' at " << log_message.file << "["
                  << log_message.line << "], thread #" << log_message.thread_id << ")\033[0m\n";
    }
    std::cout.flush();
    buffer.clear();
}

LogMessageHandler LogMessageFactory::s_message_handler;
LogMessage::LEVEL LogMessageFactory::s_log_level = LogMessage::LEVEL::ALL;

NOTF_CLOSE_NAMESPACE

#if 0
NOTF_USING_NAMESPACE;

using Clock = std::chrono::high_resolution_clock;

static constexpr uint operations = 250;
static constexpr uint repeat = 1;
static constexpr uint thread_count = 3;

void produce_garbage()
{
    for (uint i = 0; i < operations; ++i) {
        log_trace << "This is garbage with a number: " << 231;
    }
}

int main()
{
    ulong throughput = 0;
    LogHandler handler(128, 200);
    install_log_message_handler(handler);
    set_log_level(LogMessage::LEVEL::WARNING);
    handler.start();

    for (uint i = 0; i < repeat; ++i) {
        std::vector<std::thread> threads;

        Clock::time_point t0 = Clock::now();

        for (uint i = 0; i < thread_count; ++i) {
            threads.push_back(std::thread(produce_garbage));
        }

        for (auto& thread : threads) {
            thread.join();
        }

        Clock::time_point t1 = Clock::now();

        milliseconds ms = std::chrono::duration_cast<milliseconds>(t1 - t0);
        auto count = ms.count();
        if (count == 0) {
            count = 1;
        }
        throughput += static_cast<ulong>((thread_count * operations) / count);
    }
    std::cout << "Throughput with " << thread_count << " threads: " << (throughput / repeat) << std::endl;

    handler.stop();
    handler.join();

    return 0;
}
#endif
