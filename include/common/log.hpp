#pragma once

/*! @file log.hpp
 * @brief Signal UI logging mechanism.
 *
 * A very powerful, header-only logging library printing out log messages to std::out.
 *
 * @section log_logging Logging
 *
 * @subsection log_optimization Compile-time optimization
 *
 * One important feature is the support compile-time switches to disable log messages under a certain level.
 * Setting those flags will cause all calls to log_* be compiled away (limitations see below).
 * This way, you are free to plaster your code with log_trace messages and be sure that they won't negatively affect
 * runtime performance, if you compile with -DSIGNAL_LOG_LEVEL=2.
 * Note that this optimization is only true for 'constexpr' log messages, so either string literals like:
 *
 * @code
 * log_trace << "this is a string literal";
 * @endcode
 *
 * or:
 * @code
 * log_trace << constexpr_function();
 * @endcode
 *
 * with:
 *
 * @code
 * constexpr const char* constexpr_function()
 * {
 *     static const char* message = "return value from a constexpr function";
 *     return message;
 * }
 * @endcode
 *
 * Non-constexpr functions might have runtime side-effects and can therefore not be discarted by the compiler.
 */

#include <iostream>
#include <mutex>
#include <sstream>
#include <thread>
#include <vector>

#include "common/devel.hpp"
#include "common/int_utils.hpp"

namespace signal {

struct LogMessage;

using std::chrono::milliseconds;
using LogMessageHandler = std::function<void(LogMessage&&)>;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/// \brief A LogMessage consists of a raw message string and additional debug information.
struct LogMessage {

    /// \brief The level of a LogMessage indicates under what circumstance the message was created.
    enum class LEVEL {
        ALL = 0,
        TRACE, // for development only
        INFO, // for documenting expected behavior
        WARNING, // for unexpected but valid behavior
        CRITICAL, // for errors that disrupt normal program flow and are noticeable by the user
        FATAL, // for critical errors, documenting what went wrong before the application crashes
        NONE,
    };

    /// \brief Level of this LogMessage;
    LEVEL level;

    /// \brief Line of the file at which this LogMessage was created.
    uint line;

    /// \brief Thread ID of the thread from which this LogMessage originates.
    std::thread::id thread_id;

    /// \brief Name of the file containing the call to create this LogMessage;
    std::string file;

    /// \brief Name of the function from which the LogMessage was created.
    std::string caller;

    /// \brief The actual message.
    std::string message;
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/// \brief Factory object creating a LogMessage instance and passing it to the handler when going out of scope.
struct LogMessageFactory {

    friend void install_log_message_handler(LogMessageHandler);
    friend void remove_log_message_handler();
    friend LogMessage::LEVEL get_log_level();
    friend void set_log_level(LogMessage::LEVEL level);

public: // methods
    /// \brief Value constructor.
    ///
    /// \param level    Level of the LogMessage;
    /// \param line     Line of the file where this constructer is called.
    /// \param file     File in which this constructor is called.
    /// \param caller   Name of the function from where this constructor is called.
    LogMessageFactory(LogMessage::LEVEL level, uint line, std::string file, std::string caller) noexcept
        : message(),
          input()
    {
        message = LogMessage{level, line, std::this_thread::get_id(), std::move(file), std::move(caller), {}};
    }

    // Forbid allocation on the heap.
    void* operator new(size_t) = delete;
    void* operator new[](size_t) = delete;
    void operator delete(void*) = delete;
    void operator delete[](void*) = delete;

    /// \brief Destructor.
    ~LogMessageFactory()
    {
        if (s_message_handler && message.level >= s_log_level) {
            // construct the message from the input stream before passing it on to the handler
            message.message = input.str();
            s_message_handler(std::move(message));
        }
    }

public: // fields
    /// \brief The constructed LogMessage.
    LogMessage message;

    /// \brief String stream to construct the message.
    std::stringstream input;

private: // static fields
    /// \brief The message handler to which all fully created LogMessages are passed before destruction.
    static LogMessageHandler s_message_handler;

    /// \brief The minimum level at which operations are logged.
    static LogMessage::LEVEL s_log_level;
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/// \brief Log message handle provider using double-buffering in a separate thread.
class LogHandler {

public: // methods
    /// \brief Default constructor.
    ///
    /// \param initial_buffer   Initial size of the buffers.
    /// \param flush_interval   How often the read- and write-buffers are swapped in milliseconds.
    explicit LogHandler(size_t initial_buffer, ulong flush_interval);

    /// \brief Logs a new message.
    ///
    /// Is thread-safe.
    ///
    /// \param message  Message to log.
    void push_log(LogMessage message)
    {
        // if the thread is running, add the log message to the write buffer
        if (m_is_running.test_and_set()) {
            std::lock_guard<std::mutex> scoped_lock(m_mutex);
            UNUSED(scoped_lock);

            bool force_flush = message.level > LogMessage::LEVEL::WARNING;
            m_write_buffer.emplace_back(std::move(message));

            // messages of level warning or higher cause an immediate (blocking) flush,
            // because the application might crash before we have the chance to properly swap the buffers
            if (force_flush) {
                flush_buffer(m_write_buffer);
            }
        }

        // otherwise do nothing
        else {
            m_is_running.clear();
        }
    }

    /// \brief Starts the handler loop in a separate thread.
    void start();

    /// \brief Stops the handler loop on the next runthrough.
    ///
    /// Does nothing if the handler is not currently running.
    void stop() { m_is_running.clear(); }

    /// \brief Call after stop() to join the handler thread.
    ///
    /// Is a separate function so you can use the time between stop() and the thread finishing.
    /// Does nothing if the handler cannot be joined.
    void join();

    /// \brief Sets the number of digits that the Log message counter should align for.
    void set_number_padding(ushort digits) { m_number_padding = digits; }

    /// \brief Colors all future log messages of the given level in the given color.
    /// \param level    Log message level to color.
    /// \param color    Which color to use, see description for details.
    ///
    /// Signal UI can color log messages with up to 256 colors.
    /// See http://misc.flogisoft.com/bash/tip_colors_and_formatting#colors1
    ///
    void set_color(LogMessage::LEVEL level, u_char color);

private: // methods
    /// \brief Thread execution function.
    void run();

    /// \brief Flushes the read buffer.
    ///
    /// Afterwards, the given buffer is empty.
    ///
    /// \param buffer   Buffer to flush.
    void flush_buffer(std::vector<LogMessage>& buffer);

private: // fields
    /// \brief Incoming messages are stored in the write buffer.
    std::vector<LogMessage> m_write_buffer;

    /// \brief The read buffer is used by the log handler thread to flush the messages.
    std::vector<LogMessage> m_read_buffer;

    /// \brief Mutex used for thread-safe access to the write log.
    std::mutex m_mutex;

    /// \brief Thread in which the hander loop is run.
    std::thread m_thread;

    /// \brief Counter, assigning a unique ID to each log message.
    ulong m_log_count;

    /// \brief How often the read- and write-buffers are swapped in milliseconds.
    milliseconds m_flush_interval;

    /// \brief Flag indicating if the handler loop shoud continue or not.
    std::atomic_flag m_is_running = ATOMIC_FLAG_INIT; // set to false, see https://stackoverflow.com/a/24438336

    /// \brief The number of digits that the message should align for.
    /// If '3', single digit numbers will be padded with two spaces to the left, double-digits with a single space.
    ushort m_number_padding;

    // \brief Terminal color value of trace log messages.
    uchar m_color_trace;

    // \brief Terminal color value of info log messages.
    uchar m_color_info;

    // \brief Terminal color value of warning log messages.
    uchar m_color_warning;

    // \brief Terminal color value of critical log messages.
    uchar m_color_critical;

    // \brief Terminal color value of fatal log messages.
    uchar m_color_fatal;
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/// \brief Installs a new handler function to consume all future log messages.
///
/// Without a user-defined handler, all LogMessages are immediately destroyed.
///
/// \param handler  Log message handler function.
inline void install_log_message_handler(LogMessageHandler handler)
{
    LogMessageFactory::s_message_handler = handler;
}

/// \brief Overload to install a LogHandler instance as the log message handler.
///
/// \param handler  LogHandler instance
inline void install_log_message_handler(LogHandler& handler)
{
    install_log_message_handler(std::bind(&LogHandler::push_log, &handler, std::placeholders::_1));
}

/// \brief Removes a previously installed log message handler.
///
/// All future messages are ignored until a new handler is installed.
inline void remove_log_message_handler()
{
    LogMessageFactory::s_message_handler = nullptr;
}

/// \brief The minimum log level required for a message to be logged.
///
/// \return The minimum log level required for a message to be logged.
inline LogMessage::LEVEL get_log_level()
{
    return LogMessageFactory::s_log_level;
}

/// \brief Sets the minimum log level required for a message to be logged.
///
/// \param level    The minimum log level required for a message to be logged.
inline void set_log_level(LogMessage::LEVEL level)
{
    LogMessageFactory::s_log_level = level;
}

/// \brief Extracts the last part of a pathname at compile time.
///
/// We know that this function is performed at compile time because you can initialize a 'constexpr' variable with it.
///
/// \param input        Path to investigate.
/// \param delimiter    Delimiter used to separate path elements.
///
/// \return Only the last part of the path, e.g. basename("/path/to/some/file.cpp", '/') would return "file.cpp".
constexpr const char* basename(const char* input, const char delimiter = '/')
{
    size_t last_occurrence = 0;
    for (size_t offset = 0; input[offset]; ++offset) {
        if (input[offset] == delimiter) {
            last_occurrence = offset + 1;
        }
    }
    return &input[last_occurrence];
}

/// \brief The NullBuffer is a helper class to ignore unwanted logging messages.
///
/// It is used instead of a LogMessageFactory as target for logging strings when the code was compiled with flags to
/// ignore certain levels of logging calls.
/// For example, if the code was compiled using SIGNAL_LOG_LEVEL = 3 (warnings and errors only), all log_info and
/// log_trace calls target a _NullBuffer.
/// The _NullBuffer class provides a '<<' operator for all types of inputs but just ignores the argument.
/// This way, input into a _NullBuffer is simply optimized out of existence.
struct _NullBuffer {
    template <typename ANY>
    _NullBuffer& operator<<(ANY&&) { return *this; }
};

} // namespace signal

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#define SIGNAL_LOG_LEVEL_TRACE 1
#define SIGNAL_LOG_LEVEL_INFO 2
#define SIGNAL_LOG_LEVEL_WARNING 3
#define SIGNAL_LOG_LEVEL_CRITICAL 4
#define SIGNAL_LOG_LEVEL_FATAL 5

#ifndef SIGNAL_LOG_LEVEL
#define SIGNAL_LOG_LEVEL 0 // log all messages if no value for SIGNAL_LOG_LEVEL was given
#endif

/// Define log_* macros.
///
/// Use these macros like this:
///
///     log_trace << "this is log message: " << 42;
///     log_critical << "Caught unhandled error: " << error_object;
///
/// The object provided by log_* is a std::stringstream or a _NullBuffer, which accepts all the same inputs.
#ifndef log_trace
#if SIGNAL_LOG_LEVEL <= SIGNAL_LOG_LEVEL_TRACE
#define log_trace signal::LogMessageFactory(signal::LogMessage::LEVEL::TRACE, __LINE__, signal::basename(__FILE__), __FUNCTION__).input
#else
#define log_trace signal::_NullBuffer()
#endif
#else
#warning "Macro 'log_trace' is already defined - Signal's log_trace macro will remain disabled."
#endif

#ifndef log_info
#if SIGNAL_LOG_LEVEL <= SIGNAL_LOG_LEVEL_INFO
#define log_info signal::LogMessageFactory(signal::LogMessage::LEVEL::INFO, __LINE__, signal::basename(__FILE__), __FUNCTION__).input
#else
#define log_info signal::_NullBuffer()
#endif
#else
#warning "Macro 'log_info' is already defined - Signal's log_info macro will remain disabled."
#endif

#ifndef log_warning
#if SIGNAL_LOG_LEVEL <= SIGNAL_LOG_LEVEL_WARNING
#define log_warning signal::LogMessageFactory(signal::LogMessage::LEVEL::WARNING, __LINE__, signal::basename(__FILE__), __FUNCTION__).input
#else
#define log_warning signal::_NullBuffer()
#endif
#else
#warning "Macro 'log_warning' is already defined - Signal's log_warning macro will remain disabled."
#endif

#ifndef log_critical
#if SIGNAL_LOG_LEVEL <= SIGNAL_LOG_LEVEL_CRITICAL
#define log_critical signal::LogMessageFactory(signal::LogMessage::LEVEL::CRITICAL, __LINE__, signal::basename(__FILE__), __FUNCTION__).input
#else
#define log_critical signal::_NullBuffer()
#endif
#else
#warning "Macro 'log_critical' is already defined - Signal's log_critical macro will remain disabled."
#endif

#ifndef log_fatal
#if SIGNAL_LOG_LEVEL <= SIGNAL_LOG_LEVEL_FATAL
#define log_fatal signal::LogMessageFactory(signal::LogMessage::LEVEL::FATAL, __LINE__, signal::basename(__FILE__), __FUNCTION__).input
#else
#define log_fatal signal::_NullBuffer()
#endif
#else
#warning "Macro 'log_fatal' is already defined - Signal's log_fatal macro will remain disabled."
#endif
