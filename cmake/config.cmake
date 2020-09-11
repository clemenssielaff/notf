# GLOBAL CONSTANTS #####################################################################################################

# Make sure that the project has a version number.
if(NOT DEFINED PROJECT_VERSION_MAJOR)
    message(FATAL_ERROR "Please define a project version for ${PROJECT_NAME} with major.minor.patch")
endif()

# Names of the notf namespaces.
# These macro can be used to implement namespace versioning.
set(NOTF_NAMESPACE_NAME "notf")
set(NOTF_LITERALS_NAMESPACE_NAME "literals")

# these variables will be defined in notf/meta/config.hpp
set(NOTF_VERSION_STRING ${PROJECT_VERSION})

# CMAKE DEFAULTS #######################################################################################################

# CMake policies
if(COMMAND cmake_policy)

    # from https://cliutils.gitlab.io/modern-cmake/chapters/basics.html
    if(CMAKE_VERSION VERSION_LESS 3.12)
        cmake_policy(VERSION ${CMAKE_MAJOR_VERSION}.${CMAKE_MINOR_VERSION})
    else()
        cmake_policy(VERSION 3.12)
    endif()

    # apply modern policies
    foreach(p
        CMP0048 # it's okay to clear PROJECT_VERSION on project()
        CMP0056 # use the same linker flags for `try_compile` as for the rest of the project
        CMP0063 # honor visibility properties for all targets
        CMP0066 # use the same compile flags for `try_compile` as for the rest of the project
        CMP0067 # use the same c++ standard for `try_compile` as for the rest of the project
        CMP0072 # OpenGL prefers GLVND by default when available
        )
      if(POLICY ${p})
        cmake_policy(SET ${p} NEW)
      endif()
    endforeach()

    # Only interpret if() arguments as variables or keywords when unquoted.
    if (NOT CMAKE_VERSION VERSION_LESS "3.1")
        cmake_policy(SET CMP0054 NEW)
    endif()

endif()

# print additional information during compilation (like timings)
set(CMAKE_VERBOSE_MAKEFILE ON CACHE BOOL "ON")

# C++ DEFAULTS #########################################################################################################

# use plain c++ 17
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# hide symbols by default
set(CMAKE_CXX_VISIBILITY_PRESET hidden)
set(CMAKE_VISIBILITY_INLINES_HIDDEN 1)

# define notf's debug/release flags
if(CMAKE_BUILD_TYPE MATCHES "Release")
    message(STATUS "${PROJECT_NAME}: Building in release mode")
    set(NOTF_RELEASE TRUE)
else()
    message(STATUS "${PROJECT_NAME}: Building in debug mode")
    set(NOTF_DEBUG TRUE)
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

# use link-time optimization if available
if(CMAKE_BUILD_TYPE MATCHES "Release")
    include(CheckIPOSupported)
    check_ipo_supported(RESULT IS_LTO_SUPPORTED OUTPUT LTO_ERROR)
    if(IS_LTO_SUPPORTED)
        message(STATUS "${PROJECT_NAME}: Building with LTO ✔")
        set_property(GLOBAL PROPERTY INTERPROCEDURAL_OPTIMIZATION TRUE)
    else()
        message(STATUS "${PROJECT_NAME}: Building without LTO ✘\n\t${LTO_ERROR}")
    endif()
endif()

# use LLVM libraries and tools when building with clang
if (NOTF_CLANG)
    message(STATUS "${PROJECT_NAME}: Using llvm tools: libc++ and lld ✔")
    add_compile_options("-stdlib=libc++")
    add_link_options(
        "-stdlib=libc++"
        "-lc++abi"
        "-fuse-ld=lld"
    )
endif()

# use ccache if available
find_program(NOTF_CCACHE_PROGRAM ccache)
if(NOTF_CCACHE_PROGRAM)
    message(STATUS "${PROJECT_NAME}: Using ccache ✔")
    set_property(GLOBAL PROPERTY RULE_LAUNCH_COMPILE ${NOTF_CCACHE_PROGRAM})
endif()

# compiler feature check
include(try_compiles)

# GLOBAL OPTIONS #######################################################################################################

# log level
set(NOTF_LOG_LEVEL "0" CACHE STRING "Log level to compile (0=all -> 6=nothing)")

# abort on assert
option(NOTF_ABORT_ON_ASSERT "Abort straight away when failing an assert" 1)
if(NOTF_ABORT_ON_ASSERT)
    message(STATUS "${PROJECT_NAME}: Aborting on assert failure ✔")
    set(NOTF_ABORT_ON_ASSERT 1)
endif()

# coverage
if(NOTF_DEBUG)
    option(NOTF_CREATE_COVERAGE "Build code with coverage" OFF)
endif()

# CONFIGURATION DEPENDENT GLOBALS ######################################################################################

# create coverage for the core library
if(NOTF_CREATE_COVERAGE)
    message(STATUS "${PROJECT_NAME}: Building with coverage ✔")
    if (NOTF_CLANG)
        set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -fprofile-instr-generate -fcoverage-mapping")
        set(CMAKE_EXE_LINKER_FLAGS_DEBUG "${CMAKE_EXE_LINKER_FLAGS_DEBUG} -fprofile-instr-generate")
    else()
        set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} --coverage")
    endif()
endif()

# SYSTEM DEPENDEND GLOBALS #############################################################################################

# global definitions
if(NOTF_WINDOWS)
    set(NOTF_GLOBAL_DEFINES
        WIN32_LEAN_AND_MEAN # Don't include most of Windows.h
        NOGDI
        NOMINMAX
    )
else()
    set(NOTF_GLOBAL_DEFINES
        # empty
    )
endif()
if(NOTF_TEST)
    set(NOTF_GLOBAL_DEFINES ${NOTF_GLOBAL_DEFINES}
        NOTF_TEST
    )
endif()

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

# MACROS ###############################################################################################################

# utility macro to add source files in a common subdirectory
macro(add_sources source_list base_path)
    foreach(file ${ARGN})
        list(APPEND ${source_list} ${base_path}/${file})
    endforeach()
endmacro()
