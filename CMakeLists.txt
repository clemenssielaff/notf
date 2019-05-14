# require at least CMake 3.9 to support Interprocedural Optimization IPO
cmake_minimum_required(VERSION 3.9)

# top-level project
project(notf
    VERSION 0.4.0.0
    DESCRIPTION "Reactive & Lightweight UI Application Engine in C++/Python"
    LANGUAGES CXX
)

# apply modern policies
if(COMMAND cmake_policy)
    cmake_policy(PUSH)

    # from https://cliutils.gitlab.io/modern-cmake/chapters/basics.html
    if(CMAKE_VERSION VERSION_LESS 3.12)
        cmake_policy(VERSION ${CMAKE_MAJOR_VERSION}.${CMAKE_MINOR_VERSION})
    else()
        cmake_policy(VERSION 3.12)
    endif()

    # Only interpret if() arguments as variables or keywords when unquoted.
    if (NOT CMAKE_VERSION VERSION_LESS "3.1")
        cmake_policy(SET CMP0054 NEW)
    endif()

    # Use the same linker flags for `try_compile` as for the rest of the project.
    if(POLICY CMP0056)
        cmake_policy(SET CMP0056 NEW)
    endif()

    # Use the same compile flags for `try_compile` as for the rest of the project.
    if(POLICY CMP0066)
        cmake_policy(SET CMP0066 NEW)
    endif()

    # Use the same c++ standard for `try_compile` as for the rest of the project.
    if(POLICY CMP0067)
        cmake_policy(SET CMP0067 NEW)
    endif()

    # FindOpenGL prefers GLVND by default when available.
    if(POLICY CMP0072)
        cmake_policy(SET CMP0072 NEW)
    endif()

endif()

# SYSTEM CHECK #########################################################################################################

# check compiler ID (from https://cmake.org/cmake/help/latest/variable/CMAKE_LANG_COMPILER_ID.html)
if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    set(NOTF_CLANG TRUE)
elseif (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    set(NOTF_GCC TRUE)
elseif (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    set(NOTF_MSVC TRUE)
endif()

# check operating system (from https://stackoverflow.com/a/40152725)
if(WIN32)
    set(NOTF_WINDOWS TRUE)
elseif(APPLE)
    set(NOTF_MACINTOSH TRUE)
elseif(UNIX)
    set(NOTF_LINUX TRUE)
endif()

# check endianness
include (TestBigEndian)
TEST_BIG_ENDIAN(NOTF_IS_BIG_ENDIAN)

# PROJECT ##############################################################################################################

# cmake modules
set (CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")
find_package(Threads REQUIRED)
find_package(OpenGL REQUIRED)

# use plain c++ 17
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# hide symbols by default
set(CMAKE_CXX_VISIBILITY_PRESET hidden)
set(CMAKE_VISIBILITY_INLINES_HIDDEN 1)

# use link-time optimization if available
if(CMAKE_BUILD_TYPE MATCHES "Release")
    include(CheckIPOSupported)
    check_ipo_supported(RESULT IS_LTO_SUPPORTED OUTPUT LTO_ERROR)
    if(IS_LTO_SUPPORTED)
        message(STATUS "notf: Building with LTO")
        set_property(GLOBAL PROPERTY INTERPROCEDURAL_OPTIMIZATION TRUE)
    else()
        message(STATUS "notf:Building without LTO:\n${LTO_ERROR}")
    endif()
endif()

# use LLVM libraries and tools
if (NOTF_CLANG)
    message(STATUS "notf: Using llvm tools: libc++ and lld ")
    add_compile_options("-stdlib=libc++")
    add_link_options(
        "-stdlib=libc++"
        "-lc++abi"
        "-fuse-ld=lld"
    )
endif()

# use ccache if available
find_program(CCACHE_PROGRAM ccache)
if(CCACHE_PROGRAM)
    message(STATUS "notf: Using ccache")
    set_property(GLOBAL PROPERTY RULE_LAUNCH_COMPILE ${CCACHE_PROGRAM})
endif()

# CONFIGURATION ########################################################################################################

# compiler feature check
include(try_compiles)

# define notf's debug/release flags
if(CMAKE_BUILD_TYPE MATCHES "Release")
    message(STATUS "notf: Building in release mode")
    set(NOTF_RELEASE TRUE)
else()
    message(STATUS "notf: Building in debug mode")
    set(NOTF_DEBUG TRUE)
endif()

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

# build the tests
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

# build the apps
option(NOTF_BUILD_APPS "Build apps" OFF)
if(NOTF_BUILD_APPS)
    message(STATUS "notf: Building apps.")
else()
    message(STATUS "notf: NOT building apps.")
endif()

# build the benchmarks
option(NOTF_BUILD_BENCHMARKS "Build benchmarks" OFF)
if(NOTF_BUILD_BENCHMARKS)
    message(STATUS "notf: Building benchmarks")
endif()

# log level
set(NOTF_LOG_LEVEL "0" CACHE STRING "Log level to compile (0=all -> 6=nothing)")

# abort on assert
option(NOTF_ABORT_ON_ASSERT "Abort straight away when failing an assert" 1)
if(NOTF_ABORT_ON_ASSERT)
    message(STATUS "notf: Aborting on assert failure")
    set(NOTF_ABORT_ON_ASSERT 1)
endif()

# CODE INTEGRATION #####################################################################################################

# Name of the notf namespace.
# This macro can be used to implement namespace versioning.
set(NOTF_NAMESPACE_NAME "notf")

# these variables will be defined in notf/meta/config.hpp
set(NOTF_VERSION_STRING ${PROJECT_VERSION})
set(NOTF_LITERALS_NAMESPACE_NAME "literals")

configure_file (
    "${PROJECT_SOURCE_DIR}/include/notf/meta/config.hpp.in"
    "${CMAKE_BINARY_DIR}/config/notf/meta/config.hpp"
)
include_directories("${CMAKE_BINARY_DIR}/config")

# SYSTEM DEPENDEND GLOBALS ############################################################################################

# global definitions
if(NOTF_WINDOWS)
    set(NOTF_GLOBAL_DEFINES
        WIN32_LEAN_AND_MEAN # Don't include most of Windows.h
        NOGDI
        NOMINMAX
    )
else()
    set(NOTF_GLOBAL_DEFINES
    )
endif()
if(NOTF_TEST)
    set(NOTF_GLOBAL_DEFINES ${NOTF_GLOBAL_DEFINES}
        NOTF_TEST
    )
endif()
set(NOTF_GLOBAL_DEFINES ${NOTF_GLOBAL_DEFINES}
    BOOST_ALL_NO_LIB # do not try to auto link boost libraries, we've got them all in notf-thirdparty
)

# global options
if(NOTF_CLANG)
    set(NOTF_GLOBAL_OPTIONS
#        -Weverything
        -Wno-weak-vtables
        -Wno-covered-switch-default

        # clang vectorization analysis
#        -Rpass-missed=loop-vectorized
#        -Rpass-analysis=loop-vectorize
#        -fsave-optimization-record
#        -gline-tables-only
#        -gcolumn-info
    )
elseif(NOTF_GCC)
    set(NOTF_GLOBAL_OPTIONS
        -Wall
        -Wno-unknown-pragmas
    )
elseif(NOTF_MSVC)
    set(NOTF_GLOBAL_OPTIONS
        /W4
        /EHs # use instead of /EHsc, tell the compiler that functions declared as extern "C" may throw an exception
             # do *not* use `/GL` (global program optimization) as that would interfere with boost::fiber
    )
endif()

# SUB-PROJECTS #########################################################################################################

# utility macro to add source files in a common subdirectory
macro(add_sources source_list base_path)
    foreach(file ${ARGN})
        list(APPEND ${source_list} ${base_path}/${file})
    endforeach()
endmacro()

# discover all notf header files for use in subprojects
file(GLOB_RECURSE NOTF_HEADERS
    "include/notf/*.hpp"
)

# additional markdown files for documentation
file(GLOB_RECURSE NOTF_DOCS
    "include/notf/*.md"
)

add_subdirectory(thirdparty)
add_subdirectory(src)

if(NOTF_BUILD_APPS)
    add_subdirectory(apps)
endif()

if(NOTF_BUILD_TESTS)
    add_subdirectory(tests)
endif()

if(NOTF_BUILD_BENCHMARKS)
    add_subdirectory(benchmarks)
endif()

########################################################################################################################

if(COMMAND cmake_policy)
    cmake_policy(POP)
endif()
