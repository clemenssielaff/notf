#include "common/log.hpp"

#include "common/devel.hpp"

namespace signal {

LogHandler::LogHandler(size_t initial_buffer, ulong flush_interval)
    : m_write_buffer()
    , m_read_buffer()
    , m_mutex()
    , m_thread()
    , m_log_count(0)
    , m_flush_interval{ flush_interval }
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
    static const std::string DEBUG =    "debug:    ";
    static const std::string INFO =     "info:     ";
    static const std::string WARNING =  "warning:  ";
    static const std::string CRITICAL = "critical: ";
    static const std::string FATAL =    "fatal:    ";

    for (auto& log_message : buffer) {

        // id
        std::cout << ++m_log_count << ". ";

        // level
        switch (log_message.level) {
        case LogMessage::LEVEL::ALL:
        case LogMessage::LEVEL::DEBUG:
            std::cout << DEBUG;
            break;
        case LogMessage::LEVEL::INFO:
            std::cout << INFO;
            break;
        case LogMessage::LEVEL::WARNING:
            std::cout << WARNING;
            break;
        case LogMessage::LEVEL::CRITICAL:
            std::cout << CRITICAL;
            break;
        case LogMessage::LEVEL::FATAL:
            std::cout << FATAL;
            break;
        case LogMessage::LEVEL::NONE:
            continue;
        }

        // message
        std::cout << log_message.message
                  << " (from '" << log_message.caller
                  << "' at " << log_message.file
                  << "[" << log_message.line
                  << "], thread #" << log_message.thread_id
                  << ")\n";
    }
    std::cout.flush();
    buffer.clear();
}

LogMessageHandler LogMessageFactory::s_message_handler;
LogMessage::LEVEL LogMessageFactory::s_log_level = LogMessage::LEVEL::ALL;

} // namespace signal

#if 0
using namespace signal;

using Clock = std::chrono::high_resolution_clock;

static const uint operations = 250;
static const uint repeat = 1;
static const uint thread_count = 3;

void produce_garbage()
{
    for (uint i = 0; i < operations; ++i) {
        log_debug << "This is garbage with a number: " << 231;
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
