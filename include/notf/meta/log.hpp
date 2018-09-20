#pragma once

#include "../meta/debug.hpp"
#include "../meta/macros.hpp"

#include "spdlog/sinks/basic_file_sink.h"
#include "spdlog/sinks/stdout_color_sinks.h"

NOTF_OPEN_META_NAMESPACE

// the logger ======================================================================================================= //

/// notf logger singleton
class TheLogger {

    // types -------------------------------------------------------------------------------------------------------- //
public:
    /// Log levels in ascending order.
    enum class Level {
        TRACE = spdlog::level::trace,
        DEBUG = spdlog::level::debug,
        INFO = spdlog::level::info,
        WARNING = spdlog::level::warn,
        ERROR = spdlog::level::err,
        CRITCAL = spdlog::level::critical,
        OFF = spdlog::level::off,
    };

    /// Arguments passed to the Logger on creation.
    struct Args {
        /// Name of the Logger singleton.
        std::string name = "notf";

        /// Name of the log file to write into, leave empty to disable file logging.
        std::string file_name = {};

        /// Initial log level for the logger itself.
        Level log_level = [] {
            if constexpr (meta::is_debug_build())
                return Level::TRACE;
            else
                return Level::INFO;
        }();

        /// Log level for the console logger.
        Level console_level = [] {
            if constexpr (meta::is_debug_build())
                return Level::TRACE;
            else
                return Level::WARNING;
        }();

        /// Log level for the file logger (is ignored if file logging is disabled).
        Level file_level = [] {
            if constexpr (meta::is_debug_build())
                return Level::TRACE;
            else
                return Level::INFO;
        }();

        /// If true, the log file will be cleared before the logger writes into it.
        /// If false, the logger will append to the existing content in the log file.
        bool clear_file = false;
    };

    // methods ------------------------------------------------------------------------------------------------------ //
private:
    /// Constructor.
    /// @param args     Construction arguments pack.
    TheLogger(const Args& args) : m_console_sink(std::make_shared<decltype(m_console_sink)::element_type>())
    {
        m_console_sink->set_level(static_cast<spdlog::level::level_enum>(args.console_level));
        m_console_sink->set_pattern("%i %l: %v");

        if (args.file_name.empty()) {
            m_logger = std::make_unique<decltype(m_logger)::element_type>(args.name, m_console_sink);
        }
        else {
            m_file_sink = std::make_shared<decltype(m_file_sink)::element_type>(args.file_name, args.clear_file);
            m_file_sink->set_level(static_cast<spdlog::level::level_enum>(args.file_level));
            m_file_sink->set_pattern("[%d-%m-%C %T.%e] %l: %v");

            m_logger = std::make_unique<decltype(m_logger)::element_type>(
                args.name, spdlog::sinks_init_list{m_console_sink, m_file_sink});
        }
        m_logger->set_level(static_cast<spdlog::level::level_enum>(args.log_level));
    }

public:
    /// Destructor.
    ~TheLogger() = default;

    /// Initializes the Logger if this is the first time `initialize` or `get` is called - otherwise acts like `get`.
    static TheLogger& initialize(const Args& args) { return _get(args); }

    /// Access to the Logger singleton.
    static TheLogger& get()
    {
        static Args default_args;
        return _get(default_args);
    }

    /// Access to the managed spdlog logger.
    spdlog::logger* operator->() { return m_logger.get(); }

public:
    /// Static instance provider.
    static TheLogger& _get(const Args& args)
    {
        static TheLogger instance(args);
        return instance;
    }

    // members ------------------------------------------------------------------------------------------------------ //
private:
    /// Console sink for the logger.
    std::shared_ptr<spdlog::sinks::stdout_color_sink_mt> m_console_sink;

    /// Log file sink, is empty if no path was defined in the arguments.
    std::shared_ptr<spdlog::sinks::basic_file_sink_mt> m_file_sink;

    /// The logger managed by this instance.
    std::unique_ptr<spdlog::logger> m_logger;
};

// log macros ======================================================================================================= //

/// We are using macros here instead of constexpr functions to make sure that the expression within the macro brackets
/// is never evaluated and to make use of the __FILE__ and __LINE__ macros.

#define NOTFIMPL_LOG(LEVEL, fmt, ...)                                                                                \
    ::notf::meta::TheLogger::get()->LEVEL(fmt " ({}:{})", ##__VA_ARGS__, ::notf::meta::filename_from_path(__FILE__), \
                                          __LINE__)

#if NOTF_LOG_LEVEL <= 0
#define NOTF_LOG_TRACE(fmt, ...) NOTF_DEFER(NOTFIMPL_LOG, trace, fmt, ##__VA_ARGS__)
#else
#define NOTF_LOG_TRACE(...)
#endif

#if NOTF_LOG_LEVEL <= 1
#define NOTF_LOG_DEBUG(...) NOTF_DEFER(NOTFIMPL_LOG, debug, fmt, ##__VA_ARGS__)
#else
#define NOTF_LOG_DEBUG(...)
#endif

#if NOTF_LOG_LEVEL <= 2
#define NOTF_LOG_INFO(...) NOTF_DEFER(NOTFIMPL_LOG, info, fmt, ##__VA_ARGS__)
#else
#define NOTF_LOG_INFO(...)
#endif

#if NOTF_LOG_LEVEL <= 3
#define NOTF_LOG_WARN(...) NOTF_DEFER(NOTFIMPL_LOG, warn, fmt, ##__VA_ARGS__)
#else
#define NOTF_LOG_WARN(...)
#endif

#if NOTF_LOG_LEVEL <= 4
#define NOTF_LOG_ERROR(...) NOTF_DEFER(NOTFIMPL_LOG, error, fmt, ##__VA_ARGS__)
#else
#define NOTF_LOG_ERROR(...)
#endif

#if NOTF_LOG_LEVEL <= 5
#define NOTF_LOG_CRIT(...) NOTF_DEFER(NOTFIMPL_LOG, critical, fmt, ##__VA_ARGS__)
#else
#define NOTF_LOG_CRIT(...)
#endif

NOTF_CLOSE_META_NAMESPACE
