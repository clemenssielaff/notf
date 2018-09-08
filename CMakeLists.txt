# require at least CMake 3.9 to support Interprocedural Optimization IPO
# conditional copied from https://cliutils.gitlab.io/modern-cmake/chapters/basics.html
cmake_minimum_required(VERSION 3.8)
if(${CMAKE_VERSION} VERSION_LESS 3.12)
    cmake_policy(VERSION ${CMAKE_MAJOR_VERSION}.${CMAKE_MINOR_VERSION})
else()
    cmake_policy(VERSION 3.12)
endif()

# PROJECT ##############################################################################################################

# top-level project
project(notf
    VERSION 0.4.0.0
    DESCRIPTION "Reactive & Lightweight UI Application Engine in C++/Python"
    LANGUAGES CXX
)

# use plain c++ 17
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# hide symbols by default
set(CMAKE_CXX_VISIBILITY_PRESET hidden)
set(CMAKE_VISIBILITY_INLINES_HIDDEN 1)

# define notf's debug flag when compiling a debug build
if(${CMAKE_BUILD_TYPE} MATCHES "Debug")
    add_definitions(-DNOTF_DEBUG)
endif()

# use link-time optimization if available
if(CMAKE_BUILD_TYPE MATCHES "Release")
    include(CheckIPOSupported)
    check_ipo_supported(RESULT IS_LTO_SUPPORTED OUTPUT LTO_ERROR)

    if(${IS_LTO_SUPPORTED})
#        if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
#            set(CMAKE_EXE_LINKER_FLAGS ${CMAKE_EXE_LINKER_FLAGS} "-fuse-ld=lld")
#        endif()
        message(STATUS "Building with LTO enabled")
        set_property(GLOBAL PROPERTY INTERPROCEDURAL_OPTIMIZATION TRUE)
    else()
        message(STATUS "Building without LTO:\n${LTO_ERROR}")
    endif()
endif()

# use ccache if available
find_program(CCACHE_FOUND ccache)
if(${CCACHE_FOUND})
    message(STATUS "Using ccache")
    set_property(GLOBAL PROPERTY RULE_LAUNCH_COMPILE ccache)
    set_property(GLOBAL PROPERTY RULE_LAUNCH_LINK ccache)
endif()

# apply modern policies
if(COMMAND cmake_policy)
    cmake_policy(PUSH)

    # FindOpenGL prefers GLVND by default when available.
    if(POLICY CMP0072)
        cmake_policy(SET CMP0072 NEW)
    endif()
endif()

# store information about the current git revision TODO: those should run before each compilation
find_package(Git QUIET)
if(${GIT_FOUND})
    execute_process(COMMAND ${GIT_EXECUTABLE} rev-parse --short HEAD
                    WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
                    OUTPUT_VARIABLE NOTF_BUILT_COMMIT
                    ERROR_QUIET
                    OUTPUT_STRIP_TRAILING_WHITESPACE)
    execute_process(COMMAND ${GIT_EXECUTABLE} diff --quiet
                    WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
                    RESULT_VARIABLE NOTF_BUILT_COMMIT_WAS_MODIFIED
                    ERROR_QUIET
                    OUTPUT_STRIP_TRAILING_WHITESPACE)
endif()

# CODE INTEGRATION #####################################################################################################

# these variables will be defined in notf/meta/config.hpp
set(NOTF_VERSION_STRING ${PROJECT_VERSION})

configure_file (
    "${PROJECT_SOURCE_DIR}/include/notf/meta/config.hpp.in"
    "${PROJECT_SOURCE_DIR}/include/notf/meta/config.hpp"
)

# UTILITY MACROS #######################################################################################################

# utility macro to add source files in a common subdirectory
macro(add_sources source_list base_path)
    foreach(file ${ARGN})
        list(APPEND ${source_list} ${base_path}/${file})
    endforeach()
endmacro()

# CONFIGURATION ########################################################################################################

# build and run the tests
option(NOTF_BUILD_TESTS "Build tests" OFF)
if(${NOTF_BUILD_TESTS})
    message(STATUS "Building the tests")
    enable_testing()
    add_subdirectory(tests)
endif()

# pretty (long) function names
option(NOTF_LOG_PRETTY_FUNCTION "Log long function names" ON)
if(${NOTF_LOG_PRETTY_FUNCTION})
    add_definitions(-DNOTF_LOG_PRETTY_FUNCTION)
endif()

# thread sanitization
option(NOTF_THREAD_SANITIZATION "Enable thread sanitization" OFF)
if(${NOTF_THREAD_SANITIZATION})
    if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
        add_compile_options("-fsanitize=thread")
        link_libraries("-fsanitize=thread")
        message(STATUS "Building using thread sanitization")
    else()
        set(NOTF_THREAD_SANITIZATION OFF)
        message(AUTHOR_WARNING "Thread sanitization is only supported for build using Clang")
    endif()
endif()

# SUB-PROJECTS #########################################################################################################

# discover all notf header files for use in subprojects
file(GLOB_RECURSE NOTF_HEADERS
    "include/notf/*.hpp"
)

add_subdirectory(apps)

########################################################################################################################

if(COMMAND cmake_policy)
    cmake_policy(POP)
endif()
