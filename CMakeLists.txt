cmake_minimum_required(VERSION 3.10)
project(notf)

# MACROS ###############################################################################################################

# utility macro to add source files in a common subdirectory
macro(add_sources source_list base_path)
    foreach(file ${ARGN})
        list(APPEND ${source_list} ${base_path}/${file})
    endforeach()
endmacro()

# OPTIONS ##############################################################################################################

option(NOTF_BUILD_APP "Build application" ON)
option(NOTF_BUILD_TESTS "Build tests" OFF)
option(NOTF_LOG_PRETTY_FUNCTION "Log long function names" ON)
option(NOTF_THREAD_SANITIZATION "Enable thread sanitization" OFF)

# CONFIGURATION ########################################################################################################

# c++ 17
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# hide symbols by default
set(CMAKE_CXX_VISIBILITY_PRESET hidden)
set(CMAKE_VISIBILITY_INLINES_HIDDEN 1)

# link-time optimization
include(CheckIPOSupported)
check_ipo_supported(RESULT IS_LTO_SUPPORTED OUTPUT LTO_ERROR)

# ccache
find_program(CCACHE_FOUND ccache)
if(CCACHE_FOUND)
    message(STATUS "Using ccache")
    set_property(GLOBAL PROPERTY RULE_LAUNCH_COMPILE ccache)
    set_property(GLOBAL PROPERTY RULE_LAUNCH_LINK ccache)
endif(CCACHE_FOUND)

# policies
if(COMMAND cmake_policy)
    cmake_policy(PUSH)

    if(POLICY CMP0072)
        cmake_policy(SET CMP0072 NEW)
    endif()
endif()

# thread sanitization
if(NOTF_THREAD_SANITIZATION)
    if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
        add_compile_options("-fsanitize=thread")
        link_libraries("-fsanitize=thread")
        message(STATUS "Building using thread sanitization")
    else()
        set(NOTF_THREAD_SANITIZATION OFF)
        message(AUTHOR_WARNING "Thread sanitization is only supported for build using Clang")
    endif()
endif()

# DEFINES ##############################################################################################################

set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -DNOTF_DEBUG")

if(NOTF_LOG_PRETTY_FUNCTION)
    add_definitions(-DNOTF_LOG_PRETTY_FUNCTION)
endif()

# SCRATCH ##############################################################################################################

include_directories(
    include/
)

add_sources(scratch_files src/scratch
    node_scratch.cpp
)
add_sources(scratch_files include/notf
    meta/config.hpp
    meta/dll.hpp
    meta/meta.hpp
    meta/numeric.hpp
    meta/typestring.hpp
    meta/version.hpp
)

add_executable(notf-scratch
    ${scratch_files}
)

target_link_libraries(notf-scratch
    ${CMAKE_THREAD_LIBS_INIT}
)

if(CMAKE_BUILD_TYPE MATCHES "Release")

    if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
        set(CMAKE_EXE_LINKER_FLAGS ${CMAKE_EXE_LINKER_FLAGS} "-fuse-ld=lld")
    endif()

    if(IS_LTO_SUPPORTED)
        message(STATUS "Building notf with LTO enabled")
        set_property(TARGET notf PROPERTY INTERPROCEDURAL_OPTIMIZATION TRUE)
    else()
        message(STATUS "LTO failed:\n${LTO_ERROR}")
    endif()

endif()

########################################################################################################################

if(COMMAND cmake_policy)
    cmake_policy(POP)
endif()
