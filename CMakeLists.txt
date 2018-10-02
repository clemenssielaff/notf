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

# cmake modules
set (CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")
find_package( Threads )

include(cotire)

# use plain c++ 17
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# hide symbols by default
set(CMAKE_CXX_VISIBILITY_PRESET hidden)
set(CMAKE_VISIBILITY_INLINES_HIDDEN 1)

# use link-time optimization if available
if(CMAKE_BUILD_TYPE MATCHES "Release")
    message(STATUS ">>> notf: Building in release mode")

    include(CheckIPOSupported)
    check_ipo_supported(RESULT IS_LTO_SUPPORTED OUTPUT LTO_ERROR)

    if(${IS_LTO_SUPPORTED})
        message(STATUS ">>> notf: Building with LTO")
        set_property(GLOBAL PROPERTY INTERPROCEDURAL_OPTIMIZATION TRUE)
    else()
        message(STATUS ">>> notf:Building without LTO:\n${LTO_ERROR}")
    endif()
endif()

# use lld for clang builds
if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
    message(STATUS ">>> notf: Linking with lld ")
    set(CMAKE_EXE_LINKER_FLAGS ${CMAKE_EXE_LINKER_FLAGS} "-fuse-ld=lld")
endif()

# use ccache if available
find_program(CCACHE_PROGRAM ccache)
if(CCACHE_PROGRAM)
    message(STATUS ">>> notf: Using ccache")
    set_property(GLOBAL PROPERTY RULE_LAUNCH_COMPILE ${CCACHE_PROGRAM})
endif()

# apply modern policies
if(COMMAND cmake_policy)
    cmake_policy(PUSH)

    # FindOpenGL prefers GLVND by default when available.
    if(POLICY CMP0072)
        cmake_policy(SET CMP0072 NEW)
    endif()
endif()

# store information about the current git revision
# note that this only displays the revision and modification state at the time that CMake is last run - not every time
# that the source is built
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

# UTILITY MACROS #######################################################################################################

# utility macro to add source files in a common subdirectory
macro(add_sources source_list base_path)
    foreach(file ${ARGN})
        list(APPEND ${source_list} ${base_path}/${file})
    endforeach()
endmacro()

# CONFIGURATION ########################################################################################################

# define notf's debug flags when compiling a debug build
if(${CMAKE_BUILD_TYPE} MATCHES "Debug")
    message(STATUS ">>> notf: Building in debug mode")
    set(NOTF_DEBUG TRUE)

    # build and run the tests
    option(NOTF_BUILD_TESTS "Build tests" ON)
    if(${NOTF_BUILD_TESTS})
        message(STATUS ">>> notf: Building tests")
        set(NOTF_TEST TRUE)
        enable_testing()
        add_subdirectory(tests)
    endif()

# define notf's relase flags when compiling a release build
else()
    message(STATUS ">>> notf: Building in release mode")
    set(NOTF_RELEASE TRUE)
endif()

# log level
set(NOTF_LOG_LEVEL "0" CACHE STRING "Log level to compile (0=all -> 6=nothing)")

option(NOTF_ABORT_ON_ASSERT "Abort straight away when failing an assert" ON)
if(${NOTF_ABORT_ON_ASSERT})
    message(STATUS ">>> notf: Aborting on assert failure")
    set(NOTF_ABORT_ON_ASSERT TRUE)
endif()

# pretty (long) function names
option(NOTF_LOG_PRETTY_FUNCTION "Log long function names" ON)
if(${NOTF_LOG_PRETTY_FUNCTION})
    message(STATUS ">>> notf: Using long function names")
    set(NOTF_LOG_PRETTY_FUNCTION TRUE)
endif()

# thread sanitization
if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
    option(NOTF_THREAD_SANITIZATION "Enable thread sanitization" OFF)
    if(${NOTF_THREAD_SANITIZATION})
        add_compile_options("-fsanitize=thread")
        link_libraries("-fsanitize=thread")
        message(STATUS ">>> notf: Building using thread sanitization")
    endif()
endif()

# CODE INTEGRATION #####################################################################################################

# check compiler ID (from https://cmake.org/cmake/help/latest/variable/CMAKE_LANG_COMPILER_ID.html)
if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
    set(NOTF_CLANG TRUE)
elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
    set(NOTF_GCC TRUE)
elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
    set(NOTF_MSVC TRUE)
endif()

# these variables will be defined in notf/meta/config.hpp
set(NOTF_VERSION_STRING ${PROJECT_VERSION})

configure_file (
    "${PROJECT_SOURCE_DIR}/include/notf/meta/config.hpp.in"
    "${PROJECT_SOURCE_DIR}/include/notf/meta/config.hpp"
)

# SUB-PROJECTS #########################################################################################################

# discover all notf header files for use in subprojects
file(GLOB_RECURSE NOTF_HEADERS
    "include/notf/*.hpp"
)

add_subdirectory(thirdparty)
add_subdirectory(src)
add_subdirectory(apps)

########################################################################################################################

if(COMMAND cmake_policy)
    cmake_policy(POP)
endif()
