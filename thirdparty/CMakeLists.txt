# The notf thirdparty library project
set(CURRENT_TARGET notf-thirdparty)

# PROJECT ##############################################################################################################

# discover all thirdparty header files
file(GLOB_RECURSE SOURCE_FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/include/*.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/*.h
)

# add cross-plattform source files
add_sources(SOURCE_FILES ${CMAKE_CURRENT_SOURCE_DIR}/src/
    glad/glad.cc
    msgpack11/msgpack11.cpp
    tinyutf8/tinyutf8.cpp
)

# add google benchmark library only on request
if(NOTF_BUILD_BENCHMARKS)
    message(STATUS ">>> notf: Including benchmark in thirdparty library")
    add_sources(SOURCE_FILES ${CMAKE_CURRENT_SOURCE_DIR}/src/benchmark
        benchmark_api_internal.cc
        benchmark.cc
        benchmark_main.cc
        benchmark_register.cc
        benchmark_runner.cc
        colorprint.cc
        commandlineflags.cc
        complexity.cc
        console_reporter.cc
        counter.cc
        csv_reporter.cc
        json_reporter.cc
        reporter.cc
        sleep.cc
        statistics.cc
        string_util.cc
        sysinfo.cc
        timers.cc
    )
endif()

# some libraries complain about `stdlib=libc++` and this is the only way I found to get them to stop
add_compile_options("-Wno-unused-command-line-argument" )

# declare thirdparty library
add_library(${CURRENT_TARGET} STATIC
    ${SOURCE_FILES}
)

# thirdparty library include paths
target_include_directories(${CURRENT_TARGET} PUBLIC
    ${CMAKE_SOURCE_DIR}/thirdparty/include/
)

# global defines and options
target_compile_options(${CURRENT_TARGET} PRIVATE ${NOTF_GLOBAL_OPTIONS})
target_compile_definitions(${CURRENT_TARGET} PUBLIC ${NOTF_GLOBAL_DEFINES})

# SUB-PROJECTS #########################################################################################################

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/fmt_5.3)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/boost_1.70)

# glfw
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/glfw_3.3)

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/freetype_2.10)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/lua_5.3.5)
