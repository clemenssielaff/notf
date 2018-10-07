# The notf thirdparty library project

# discover all thirdparty header files
file(GLOB_RECURSE THIRDPARTY_FILES
    "${CMAKE_CURRENT_SOURCE_DIR}/*.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.h"
)

# add thirdparty source files
add_sources(THIRDPARTY_FILES ${CMAKE_CURRENT_SOURCE_DIR}/
    fmt/format.cc
    fmt/posix.cc
)

# add google benchmark library only on request
if(${NOTF_BUILD_BENCHMARKS})
    message(STATUS ">>> notf: Including benchmark in thirdparty library")
    add_sources(THIRDPARTY_FILES ${CMAKE_CURRENT_SOURCE_DIR}/
        benchmark/src/benchmark_api_internal.cc
        benchmark/src/benchmark.cc
        benchmark/src/benchmark_main.cc
        benchmark/src/benchmark_register.cc
        benchmark/src/benchmark_runner.cc
        benchmark/src/colorprint.cc
        benchmark/src/commandlineflags.cc
        benchmark/src/complexity.cc
        benchmark/src/console_reporter.cc
        benchmark/src/counter.cc
        benchmark/src/csv_reporter.cc
        benchmark/src/json_reporter.cc
        benchmark/src/reporter.cc
        benchmark/src/sleep.cc
        benchmark/src/statistics.cc
        benchmark/src/string_util.cc
        benchmark/src/sysinfo.cc
        benchmark/src/timers.cc
    )
endif()

# declare thirdparty library
add_library(notf-thirdparty STATIC
    ${THIRDPARTY_FILES}
)

# thirdparty library include paths
target_include_directories(notf-thirdparty PUBLIC
    ${CMAKE_SOURCE_DIR}/thirdparty/
)

# CMake cannot not determine the linker language from the files, so we'll tell it
set_target_properties(notf-thirdparty PROPERTIES LINKER_LANGUAGE CXX)
