# require at least CMake 3.9 to support Interprocedural Optimization (IPO)
cmake_minimum_required(VERSION 3.9)

# top-level project
project(notf
    VERSION 0.5.0
    DESCRIPTION "notf"
    LANGUAGES CXX
)

# notf root dir
set(NOTF_ROOT_DIR ${PROJECT_SOURCE_DIR})

# include common configuration
set (CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")
include(config)


# PROJECT OPTIONS ######################################################################################################

# thread sanitization
if (NOTF_CLANG)
    option(NOTF_THREAD_SANITIZATION "Enable thread sanitization" OFF)
    if(NOTF_THREAD_SANITIZATION)
        add_compile_options("-fsanitize=thread")
        link_libraries("-fsanitize=thread")
        message(STATUS "notf: Building using thread sanitization")

        # sanitization blacklist
        add_compile_options("-fsanitize-blacklist=${CMAKE_SOURCE_DIR}/.sanitizer-blacklist")
    endif()
endif()

# tests
if(NOTF_DEBUG)
    option(NOTF_BUILD_TESTS "Build tests" ON)
    if(NOTF_BUILD_TESTS)
        message(STATUS "notf: Building tests")
        set(NOTF_TEST TRUE)

        # enable coverage
        if(NOTF_THREAD_SANITIZATION)
            message(AUTHOR_WARNING "notf: Disabling coverage to avoid interference with thread sanitization.")
        else()
            message(STATUS "notf: Creating coverage.")
            if (NOTF_CLANG)
                set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -fprofile-instr-generate -fcoverage-mapping")
                set(CMAKE_EXE_LINKER_FLAGS_DEBUG "${CMAKE_EXE_LINKER_FLAGS_DEBUG} -fprofile-instr-generate")
            elseif(NOTF_GCC)
                set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} --coverage")
                set(CMAKE_EXE_LINKER_FLAGS_DEBUG "${CMAKE_EXE_LINKER_FLAGS_DEBUG} --coverage")
            else()
                message(AUTHOR_WARNING "notf: Code coverage is only supported for GCC and clang builds")
            endif()
        endif()
    endif()
endif()


# benchmarks
option(NOTF_BUILD_BENCHMARKS "Build benchmarks" OFF)
if(NOTF_BUILD_BENCHMARKS)
    message(STATUS "notf: Building benchmarks")
endif()


# configuration file
configure_file (
    "${PROJECT_SOURCE_DIR}/notf/meta/config.hpp.in"
    "${CMAKE_BINARY_DIR}/config/notf/meta/config.hpp"
)
include_directories("${CMAKE_BINARY_DIR}/config")


# DEPENDENCIES #########################################################################################################

find_package(Threads REQUIRED)
find_package(OpenGL REQUIRED)

# thirdparty projects
add_subdirectory(thirdparty/pybind11-2.5.0)
add_subdirectory(thirdparty/fmt-6.2.0)

set(SPDLOG_FMT_EXTERNAL_HO ON CACHE BOOL "do not use bundled fmt" FORCE)
add_subdirectory(thirdparty/spdlog-1.5.0)

# SUB-PROJECTS #########################################################################################################

# discover all notf header files for use in subprojects
file(GLOB_RECURSE NOTF_HEADERS
    "notf/**.hpp"
)
list(APPEND NOTF_HEADERS ${PROJECT_SOURCE_DIR}/notf/meta/config.hpp.in)

# discover markdown files for documentation
file(GLOB_RECURSE NOTF_DOCS
    "notf/**.md"
)

# main project
add_subdirectory(notf)

# tests
if(NOTF_BUILD_TESTS)
    add_subdirectory(tests)
endif()

# benchmarks
if(NOTF_BUILD_BENCHMARKS)
    add_subdirectory(benchmarks)
endif()
