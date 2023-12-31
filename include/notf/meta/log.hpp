#pragma once

#include "notf/meta/debug.hpp"
#include "notf/meta/macros.hpp"

#include "spdlog/sinks/basic_file_sink.h"
#include "spdlog/sinks/stdout_color_sinks.h"

NOTF_OPEN_NAMESPACE

// log macros ======================================================================================================= //

/// We are using macros here instead of constexpr functions to make sure that the expression within the macro brackets
/// is never evaluated and to make use of the __FILE__ and __LINE__ macros.

#define NOTFIMPL_LOG(...) NOTF_OVERLOADED_MACRO(NOTFIMPL_LOG, __VA_ARGS__)

/// no message
#define NOTFIMPL_LOG1(LEVEL) ::notf::TheLogger::get()->LEVEL("({}:{})", ::notf::filename_from_path(__FILE__), __LINE__)

/// you can use any single value as log entry, even if it is not a string
#define NOTFIMPL_LOG2(LEVEL, MSG) \
    ::notf::TheLogger::get()->LEVEL("{} ({}:{})", MSG, ::notf::filename_from_path(__FILE__), __LINE__)

/// if you pass multiple arguments, the first one needs to be a string,
/// subsequent arguments are used to format the string using fmt
#define NOTFIMPL_LOG3(LEVEL, MSG, ...) \
    ::notf::TheLogger::get()->LEVEL(MSG " ({}:{})", ##__VA_ARGS__, ::notf::filename_from_path(__FILE__), __LINE__)

// implementation detail
#define NOTFIMPL_LOG4(LEVEL, MSG, ...) NOTFIMPL_LOG3(LEVEL, MSG, ##__VA_ARGS__)
#define NOTFIMPL_LOG5(LEVEL, MSG, ...) NOTFIMPL_LOG3(LEVEL, MSG, ##__VA_ARGS__)
#define NOTFIMPL_LOG6(LEVEL, MSG, ...) NOTFIMPL_LOG3(LEVEL, MSG, ##__VA_ARGS__)
#define NOTFIMPL_LOG7(LEVEL, MSG, ...) NOTFIMPL_LOG3(LEVEL, MSG, ##__VA_ARGS__)
#define NOTFIMPL_LOG8(LEVEL, MSG, ...) NOTFIMPL_LOG3(LEVEL, MSG, ##__VA_ARGS__)
// add new entries here, if you need to construct a really long log entry

#if NOTF_LOG_LEVEL <= 0
#define NOTF_LOG_TRACE(MSG, ...) NOTF_DEFER(NOTFIMPL_LOG, trace, MSG, ##__VA_ARGS__)
#else
#define NOTF_LOG_TRACE(...)
#endif

#if NOTF_LOG_LEVEL <= 1
#define NOTF_LOG_DEBUG(MSG, ...) NOTF_DEFER(NOTFIMPL_LOG, debug, MSG, ##__VA_ARGS__)
#else
#define NOTF_LOG_DEBUG(...)
#endif

#if NOTF_LOG_LEVEL <= 2
#define NOTF_LOG_INFO(MSG, ...) NOTF_DEFER(NOTFIMPL_LOG, info, MSG, ##__VA_ARGS__)
#else
#define NOTF_LOG_INFO(...)
#endif

#if NOTF_LOG_LEVEL <= 3
#define NOTF_LOG_WARN(MSG, ...) NOTF_DEFER(NOTFIMPL_LOG, warn, MSG, ##__VA_ARGS__)
#else
#define NOTF_LOG_WARN(...)
#endif

#if NOTF_LOG_LEVEL <= 4
#define NOTF_LOG_ERROR(MSG, ...) NOTF_DEFER(NOTFIMPL_LOG, error, MSG, ##__VA_ARGS__)
#else
#define NOTF_LOG_ERROR(...)
#endif

#if NOTF_LOG_LEVEL <= 5
#define NOTF_LOG_CRIT(MSG, ...) NOTF_DEFER(NOTFIMPL_LOG, critical, MSG, ##__VA_ARGS__)
#else
#define NOTF_LOG_CRIT(...)
#endif

// the logger ======================================================================================================= //

/// notf logger singleton
class TheLogger {

    // types ----------------------------------------------------------------------------------- //
public:
    /// Log levels in ascending order.
    enum class Level {
        ALL = spdlog::level::trace,
        TRACE = spdlog::level::trace,
        DEBUG = spdlog::level::debug,
        INFO = spdlog::level::info,
        WARNING = spdlog::level::warn,
        ERROR = spdlog::level::err,
        CRITCAL = spdlog::level::critical,
        OFF = spdlog::level::off,
    };

    /// Arguments passed to the Logger on creation.
    struct Arguments {
        /// Name of the Logger singleton.
        std::string name = "notf";

        /// Name of the log file to write into, leave empty to disable file logging.
        std::string file_name = {};

#ifdef NOTF_DEBUG
        /// Initial log level for the logger itself.
        Level log_level = Level::TRACE;

        /// Log level for the console logger.
        Level console_level = Level::TRACE;

        /// Log level for the file logger (is ignored if file logging is disabled).
        Level file_level = Level::TRACE;
#else
        Level log_level = Level::INFO;
        Level console_level = Level::INFO;
        Level file_level = Level::INFO;
#endif

        /// If true, the log file will be cleared before the logger writes into it.
        /// If false, the logger will append to the existing content in the log file.
        bool clear_file = false;
    };

private:
    // logger ============================================================== //
    class Logger {

        // methods --------------------------------------------------------------------------------- //
    public:
        /// Constructor.
        /// @param args     Construction arguments pack.
        Logger(const Arguments& args) : m_console_sink(std::make_shared<decltype(m_console_sink)::element_type>()) {
            m_console_sink->set_level(static_cast<spdlog::level::level_enum>(args.console_level));
            m_console_sink->set_pattern("%i %^%l%$: %v");

            if (args.file_name.empty()) {
                m_logger = std::make_unique<decltype(m_logger)::element_type>(args.name, m_console_sink);
            } else {
                m_file_sink = std::make_shared<decltype(m_file_sink)::element_type>(args.file_name, args.clear_file);
                m_file_sink->set_level(static_cast<spdlog::level::level_enum>(args.file_level));
                m_file_sink->set_pattern("[%d-%m-%C %T.%e] %l: %v");

                m_logger = std::make_unique<decltype(m_logger)::element_type>(
                    args.name, spdlog::sinks_init_list{m_console_sink, m_file_sink});
            }
            m_logger->set_level(static_cast<spdlog::level::level_enum>(args.log_level));
        }

        /// Access to the managed spdlog logger.
        spdlog::logger* operator->() { return m_logger.get(); }

        /// Sets level for the Logger itself.
        void set_level(const Level level) { m_logger->set_level(static_cast<spdlog::level::level_enum>(level)); }

        /// Sets level for the console output of the Logger.
        void set_console_level(const Level level) {
            m_console_sink->set_level(static_cast<spdlog::level::level_enum>(level));
        }

        /// Sets level for the file output of the Logger (if a log file has been specified).
        void set_file_level(const Level level) {
            if (m_file_sink) {
                m_file_sink->set_level(static_cast<spdlog::level::level_enum>(level));
            } else {
                NOTF_LOG_WARN("Cannot change file log level since no log file was specified during initialization");
            }
        }

        // fields ---------------------------------------------------------------------------------- //
    private:
        /// Console sink for the logger.
        std::shared_ptr<spdlog::sinks::stdout_color_sink_mt> m_console_sink;

        /// Log file sink, is empty if no path was defined in the arguments.
        std::shared_ptr<spdlog::sinks::basic_file_sink_mt> m_file_sink;

        /// The logger managed by this instance.
        std::unique_ptr<spdlog::logger> m_logger;
    };

    // methods --------------------------------------------------------------------------------- //
public:
    /// Initializes the Logger if this is the first time `initialize` or `get` is called - otherwise acts like `get`.
    static Logger& initialize(const Arguments& args) { return _get(args); }

    /// Access to the Logger singleton.
    static Logger& get() {
        static Arguments default_args;
        return _get(default_args);
    }

public:
    /// Static instance provider.
    static Logger& _get(const Arguments& args) {
        static Logger instance(args);
        return instance;
    }

    // fields ---------------------------------------------------------------------------------- //
private:
    /// Logger instance managed by this singleton.
};

NOTF_CLOSE_NAMESPACE
