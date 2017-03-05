#pragma once

#include <chrono>
#include <iomanip>
#include <iostream>
#include <ostream>

namespace notf {

/// @brief Simple RAII timer for timing a single function call.
///
/// Implemented from:
///     http://felix.abecassis.me/2011/09/cpp-timer-raii/
/// with some improvements from:
///     http://stackoverflow.com/questions/21588063/get-the-running-time-of-a-function-in-c
///
/// Use by using the DEBUG_TIMER inside a codeblock.
/// For example:
///     {
///         DebugTimer __timer("foo");
///         ...
///     }
class DebugTimer {

public: // methods
    /// @brief Constructor
    ///
    /// @param [in] name    Name of the DebugTimer to identify the output.
    DebugTimer(const std::string& name)
        : m_name(name)
        , m_start(std::chrono::high_resolution_clock::now())
    {
    }

    /// @brief Destructor.
    ~DebugTimer()
    {
        std::chrono::high_resolution_clock::time_point end = std::chrono::high_resolution_clock::now();
        auto nanoseconds = std::chrono::duration_cast<std::chrono::nanoseconds>(end - m_start).count();
        auto milliseconds = nanoseconds / 1000000.;
        std::cout << m_name << ": " << std::setprecision(10) << milliseconds << "ms" << std::endl;
    }

private: // fields
    /// @brief Name of the DebugTimer to identify the output.
    std::string m_name;

    /// @brief High-precision construction time of the timer.
    std::chrono::high_resolution_clock::time_point m_start;
};

} // namespace notf
