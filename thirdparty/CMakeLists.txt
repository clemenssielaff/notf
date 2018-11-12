# The notf thirdparty library project
set(CURRENT_TARGET notf-thirdparty)

# discover all thirdparty header files
file(GLOB_RECURSE THIRDPARTY_FILES
    "${CMAKE_CURRENT_SOURCE_DIR}/*.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.h"
)

# add thirdparty source files
add_sources(THIRDPARTY_FILES ${CMAKE_CURRENT_SOURCE_DIR}/
    glad/glad.cc

    fmt/format.cc
    fmt/posix.cc

    msgpack11/msgpack11.cpp
)

# add google benchmark library only on request
if(NOTF_BUILD_BENCHMARKS)
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
add_library(${CURRENT_TARGET} STATIC
    ${THIRDPARTY_FILES}
)

# thirdparty library include paths
target_include_directories(${CURRENT_TARGET} PUBLIC
    ${CMAKE_SOURCE_DIR}/thirdparty/
)

# global defines and options
target_compile_options(${CURRENT_TARGET} PRIVATE ${NOTF_GLOBAL_OPTIONS})
target_compile_definitions(${CURRENT_TARGET} PUBLIC ${NOTF_GLOBAL_DEFINES})

# CMake cannot not determine the linker language from the files, so we'll tell it
set_target_properties(${CURRENT_TARGET} PROPERTIES LINKER_LANGUAGE CXX)

# SUB-PROJECTS #########################################################################################################

# glfw
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
add_subdirectory(./glfw-3.3.0)
